// Hello World client
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <math.h>

#define WIDTH 480
#define HEIGHT 70

#define CMD_PIXEL 0
#define CMD_BLIT 1

struct msgSetPixel {
  uint8_t cmd;
  uint32_t x;
  uint32_t y;
  uint8_t on;
} __attribute__ ((packed));

struct msgBlit {
  uint8_t cmd;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
  uint8_t data[0];
} __attribute__ ((packed));

int main (void) {
  void *context = zmq_ctx_new ();
  void *requester;

  requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect(requester, "tcp://mensadisplay:5556");
  // zmq_connect(requester, "tcp://192.168.178.147:5570");
  // zmq_connect(requester, "tcp://localhost:5556");

  msgBlit *snow = reinterpret_cast<msgBlit *>(malloc(sizeof(msgBlit) + WIDTH * HEIGHT));
  snow->cmd = CMD_BLIT;
  snow->x = 0;
  snow->y = 0;
  snow->w = WIDTH;
  snow->h = HEIGHT;

  float f[HEIGHT / 7] = { 0 };
  float dir[HEIGHT / 7] = { 0 };
  float sharpness[HEIGHT / 7] = { 0 };

  for(int i = 0; i < HEIGHT / 7; ++i) {
    dir[i] = 0.2;
    sharpness[i] = 2;
  }

  while(1) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(msgBlit) + WIDTH * HEIGHT);
    for(int i = 0; i < WIDTH * HEIGHT; ++i) {
      int x = i % WIDTH;
      int y = i / WIDTH;

      int thresh = 32768 + sharpness[y / 7] * sin(f[y / 7] + 1.0 * x / 60) * 32768;

      snow->data[i] = (rand() % 65536) > thresh;
    }

    for(int i = 0; i < HEIGHT / 7; ++i) {
      f[i] += dir[i];
    }

    if(!(rand() % 100)) {
      dir[rand() % (HEIGHT / 7)] *= -1;
    }
    if(!(rand() % 100)) {
      sharpness[rand() % (HEIGHT / 7)] = 1 + rand() % 3;
    }

    memcpy(zmq_msg_data(&msg), snow, sizeof(msgBlit) + WIDTH * HEIGHT);
    zmq_msg_send(&msg, requester, ZMQ_SNDMORE);

    zmq_msg_init_size(&msg, 0);
    zmq_msg_send(&msg, requester, 0);
    zmq_recv(requester, NULL, 0, 0);
  }

  zmq_close(requester);

  zmq_ctx_destroy(context);
  return 0;
}
