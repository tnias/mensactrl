#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <math.h>
#include <netinet/ip.h>
#include <iostream>
#include <sys/epoll.h>
#include <memory>
#include <algorithm>
#include <sys/time.h>
#include <set>

#define WIDTH 480
#define HEIGHT 70

#define CMD_PIXEL 0
#define CMD_BLIT 1

#define BOMB_LOAD_TIME 1000

struct msgBlit {
  uint8_t cmd;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
  uint8_t data[WIDTH * HEIGHT];
} __attribute__ ((packed));

volatile msgBlit *screen;
volatile msgBlit *offscreen;
int epollFd;
int frameNo;

void *blitScreen(void *) {
  void *context = zmq_ctx_new ();
  void *requester;

  requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect(requester, "tcp://mensadisplay:5556");
  // zmq_connect(requester, "tcp://192.168.178.147:5570");
  // zmq_connect(requester, "tcp://localhost:5556");

  while(1) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(msgBlit));

    memcpy(zmq_msg_data(&msg), const_cast<msgBlit *>(screen), sizeof(msgBlit));
    zmq_msg_send(&msg, requester, ZMQ_SNDMORE);

    zmq_msg_init_size(&msg, 0);
    zmq_msg_send(&msg, requester, 0);
    zmq_recv(requester, NULL, 0, 0);
  }

  zmq_close(requester);
  zmq_ctx_destroy(context);

  return nullptr;
}

void setPixel(int x, int y, bool on) {
  if(x < 0 || x >= WIDTH) return;
  if(y < 0 || y >= HEIGHT) return;

  offscreen->data[WIDTH * y + x] = on? 255: 0;
}

void clearScreen() {
  for(int i = 0; i < WIDTH * HEIGHT; ++i) offscreen->data[i] = 0;
}

void flipScreen() {
  volatile msgBlit *tmp = offscreen;
  screen = offscreen;
  offscreen = tmp;
}

class Actor;

std::vector<Actor *> friends;
std::vector<Actor *> enemies;
std::vector<Actor *> newFriends;
std::vector<Actor *> newEnemies;

std::set<Actor *> killed;

class Actor {
  public:
    float x, y;

    virtual void draw() = 0;
    virtual void act() = 0;
    virtual void kill() {
      killed.insert(this);
    }

    Actor(float x, float y): x(x), y(y) { }

    virtual ~Actor() { }
};

class FriendShot: public Actor {
  float dx, dy;

  public:
    FriendShot(float x, float y, float dx, float dy): Actor(x, y), dx(dx), dy(dy) { }

    void draw() {
      setPixel(x, y, true);
    }

    void act() {
      if(x < 0 || x > WIDTH) kill();
      if(y < 0 || y > HEIGHT) kill();
      
      for(auto &a: enemies) {
        float dx = a->x - x;
        float dy = a->y - y;

        if(dx * dx + dy * dy < 7) {
          a->kill();
        }
      }

      x += dx;
      y += dy;
    }
};

class Bomb: public Actor {
  float dx, dy;

  public:
    Bomb(float x, float y, float dx, float dy): Actor(x, y), dx(dx), dy(dy) { }

    void draw() {
      setPixel(x - 1, y, true);
      setPixel(x, y - 1, true);
      setPixel(x + 1, y, true);
      setPixel(x, y + 1, true);
      setPixel(x, y, true);
    }

    void act() {
      if(x < 0 || x > WIDTH) kill();
      if(y < 0 || y > HEIGHT) kill();
      
      x += dx;
      y += dy;

      if(x > WIDTH / 2 && frameNo % 5 == 0) {
        for(float v = 0.5; v < 2; v += 0.1) {
          for(float f = 0; f < 2 * M_PI; f += 0.02) {
            newFriends.push_back(new FriendShot(x, y, v * sin(f), v * cos(f)));
          }
        }
        killed.insert(this);
      }
    }

    void kill() { }
};

class Player: public Actor {
  int fd;
  int bombReload;

  public:
    Player(int fd): Actor(0, 39), fd(fd), bombReload(0) {
      x = 10;

      for(auto &a: friends) {
        Player *p = dynamic_cast<Player *>(a);
        if(p && p->x == x) x += 5;
      }
    }

    int con() {
      return fd;
    }

   void draw() {
      if(bombReload > BOMB_LOAD_TIME && frameNo % 3 == 0) return;

      setPixel(x - 2, y - 2, true);
      setPixel(x - 2, y - 1, true);
      setPixel(x - 2, y    , true);
      setPixel(x - 2, y + 1, true);
      setPixel(x - 2, y + 2, true);

      setPixel(x - 1, y - 1, true);
      setPixel(x - 1, y    , true);
      setPixel(x - 1, y + 1, true);

      setPixel(x    , y    , true);
    }

    void act() {
      ++bombReload;
    }

    void handleInput(char c[16]) {
      std::cout << (int)c[0] << ","
                << (int)c[1] << ","
                << (int)c[2] << ","
                << (int)c[3] << std::endl;
      if(c[0] == 27) {
        if(c[2] == 65) y -= 5;
        if(c[2] == 66) y += 5;
        if(y < 0) y = 0;
        if(y >= HEIGHT) y = HEIGHT - 1;
      }
      if(c[0] == 32) {
        newFriends.push_back(new FriendShot(x, y, 2, 0));
      }
      if(c[0] == 'b' && bombReload > BOMB_LOAD_TIME) {
        bombReload = 0;
        newFriends.push_back(new Bomb(x, y, 1, 0));
      }
    }

    void killConnection() {
      epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
      close(fd);
    }
};

class EnemyShot: public Actor {
  float dx, dy;

  public:
    EnemyShot(float x, float y, float dx, float dy): Actor(x, y), dx(dx), dy(dy) { }

    void draw() {
      setPixel(x, y, true);
    }

    void act() {
      if(x < 0 || x > WIDTH) kill();
      if(y < 0 || y > HEIGHT) kill();

      for(auto &a: friends) {
        float dx = a->x - x;
        float dy = a->y - y;

        if(dx * dx + dy * dy < 7) {
          Player *p = dynamic_cast<Player *>(a);
          if(p) p->killConnection();

          a->kill();
        }
      }
      
      x += dx;
      y += dy;
    }
};

class Ufo: public Actor {
  public:
    Ufo(): Actor(500, rand() % 70) { }

    void draw() {
      for(int xx = x - 2; xx < x + 3; ++xx) {
        for(int yy = y - 2; yy < y + 3; ++yy) {
          setPixel(xx, yy, true);
        }
      }

      setPixel(x, y, false);
    }

    void act() {
      if(x < -5) {
        kill();
        return;
      }

      if(rand() % 25 == 0) {
        newEnemies.push_back(new EnemyShot(x, y, -2, (rand() % 1000 - 500.0) / 500));
      }

      --x;
    }
};

class Ufo2: public Actor {
  public:
    Ufo2(): Actor(500, rand() % 70) { }

    void draw() {
      for(int xx = x - 3; xx < x + 4; ++xx) {
        for(int yy = y - 3; yy < y + 4; ++yy) {
          setPixel(xx, yy, true);
        }
      }

      setPixel(x, y, false);
    }

    void act() {
      if(x < -5) {
        kill();
        return;
      }

      if((frameNo / 50) % 3 == 0) {
        int ty = y;
        int tx = 0;

        int playerCount = 0;
        for(auto &a: friends) {
          Player *p = dynamic_cast<Player *>(a);
          if(p) ++playerCount;
        }

        if(playerCount) {
          int tp = (frameNo / 50) % playerCount;
          for(auto &a: friends) {
            Player *p = dynamic_cast<Player *>(a);
            if(p && !tp--) {
              ty = p->y;
              tx = p->x;
            }
          }

          if(tx != x) {
            newEnemies.push_back(new EnemyShot(x, y, -1.5, (ty - y) / (tx - x) * -1.5));
          }
        }
      }

      x -= 0.1;
    }
};

epoll_event *epollEvent(uint32_t events, int fd) {
  static epoll_event ret;
  ret.events = events;
  ret.data.fd = fd;
  return &ret;
}

epoll_event *epollEvent(uint32_t events, Actor *a) {
  static epoll_event ret;
  ret.events = events;
  ret.data.ptr = a;
  return &ret;
}

uint64_t now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000000ull + tv.tv_usec;
}

void nextFrame() {
  ++frameNo;
  clearScreen();

  for(auto &a: friends) {
    a->draw();
    a->act();
  }

  for(auto &a: enemies) {
    a->draw();
    a->act();
  }

  if(rand() % 100 == 0) {
    newEnemies.push_back(new Ufo());
  }

  if(rand() % 1600 == 0) {
    newEnemies.push_back(new Ufo2());
  }

  for(auto &a: killed) {
    enemies.erase(std::remove(enemies.begin(), enemies.end(), a), enemies.end());
    friends.erase(std::remove(friends.begin(), friends.end(), a), friends.end());
    delete a;
  }
  killed.clear();

  for(auto &a: newFriends) friends.push_back(a);
  newFriends.clear();
  for(auto &a: newEnemies) enemies.push_back(a);
  newEnemies.clear();

  flipScreen();
}

int main() {
  screen = new msgBlit;
  screen->cmd = CMD_BLIT;
  screen->x = 0;
  screen->y = 0;
  screen->w = WIDTH;
  screen->h = HEIGHT;

  offscreen = new msgBlit;
  offscreen->cmd = CMD_BLIT;
  offscreen->x = 0;
  offscreen->y = 0;
  offscreen->w = WIDTH;
  offscreen->h = HEIGHT;

  pthread_t blitter;
  pthread_create(&blitter, nullptr, blitScreen, nullptr);

  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7000);
  addr.sin_addr.s_addr = INADDR_ANY;
  int ret = bind(serverSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  if(ret < 0) {
    std::cerr << "Cannot bind: " << strerror(errno) << std::endl;
    return 1;
  }
  listen(serverSocket, 16);

  epollFd = epoll_create(8);
  epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, epollEvent(EPOLLIN, serverSocket));

  uint64_t nextFrameTime = 0;

  while (1) {
    epoll_event ev;
    int ret = epoll_wait(epollFd, &ev, 1, 1);
    if(ret == 1) {
      if(ev.data.fd == serverSocket) {
        int con = accept(serverSocket, nullptr, 0);
        if(con < 0) {
          std::cerr << "Cannot accept: " << strerror(errno) << std::endl;
        } else {
          Player *p = new Player(con);
          newFriends.push_back(p);

          epoll_ctl(epollFd, EPOLL_CTL_ADD, con, epollEvent(EPOLLIN, p));

          unsigned char disableLinemode[] = {
            255, 253, 34,                 /* IAC DO LINEMODE */
            255, 250, 34, 1, 0, 255, 240, /* IAC SB LINEMODE MODE 0 IAC SE */
            255, 251, 1                   /* IAC WILL ECHO */
          };
          write(con, disableLinemode, sizeof(disableLinemode));
          char welcome[] = "Welcome. Cursor keys to move, Space to fire, b for bomb (only if player blinks)\r\n";
          write(con, welcome, strlen(welcome));
        }
      } else {
        char buf[16];
        Player *p = reinterpret_cast<Player *>(ev.data.ptr);

        int len = read(p->con(), buf, 16);
        if(len <= 0) {
          p->killConnection();
          p->kill();
        } else {
          p->handleInput(buf);
        }
      }
    }

    if(now() > nextFrameTime) {
      nextFrame();
      nextFrameTime = now() + 10000;
    }
  }

  return 0;
}
