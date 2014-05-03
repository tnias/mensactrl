// Hello World client
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

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

  int on = 0;

  while(1) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(msgBlit) + WIDTH * HEIGHT);
    int d = 0;

    switch(on) {
      case 0: d = 0; break;
      case 1: d = 0; break;
      case 2: d = 0; break;
      case 3: d = 0; break;
      case 4: d = 1; break;
      case 5: d = 1; break;
      case 6: d = 0; break;
      case 7: d = 1; break;
    }

    for(int i = 0; i < WIDTH * HEIGHT; ++i) {
      snow->data[i] = d;
    }

    on = (on + 1) % 8;

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
