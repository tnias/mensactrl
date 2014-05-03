// Hello World client
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <vector>

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

  int x = 0;

  msgBlit *onLine = reinterpret_cast<msgBlit *>(malloc(sizeof(msgBlit) + 70));
  onLine->cmd = CMD_BLIT;
  onLine->y = 0;
  onLine->w = 1;
  onLine->h = HEIGHT;
  for(int i = 0; i < 70; ++i) onLine->data[i] = 255;

  msgBlit *offLine = reinterpret_cast<msgBlit *>(malloc(sizeof(msgBlit) + 70));
  offLine->cmd = CMD_BLIT;
  offLine->y = 0;
  offLine->w = 1;
  offLine->h = HEIGHT;
  for(int i = 0; i < 70; ++i) offLine->data[i] = 0;

  while(1) {
    zmq_msg_t msg;
//    for(int i = 0; i < HEIGHT; ++i) {
//      zmq_msg_init_size(&msg, sizeof(msgSetPixel));
//      msgSetPixel set = { CMD_PIXEL, static_cast<uint32_t>((x + 1) % 480), static_cast<uint32_t>(i), 1 };
//      memcpy(zmq_msg_data(&msg), &set, sizeof(set));
//      zmq_msg_send(&msg, requester, ZMQ_SNDMORE);
//    }
//
//    for(int i = HEIGHT; i < HEIGHT * 2; ++i) {
//      zmq_msg_init_size(&msg, sizeof(msgSetPixel));
//      msgSetPixel set = { CMD_PIXEL, static_cast<uint32_t>(x % 480), static_cast<uint32_t>(i - HEIGHT), 0 };
//      memcpy(zmq_msg_data(&msg), &set, sizeof(set));
//      zmq_msg_send(&msg, requester, ZMQ_SNDMORE);
//    }

    zmq_msg_init_size(&msg, sizeof(msgBlit) + 70);
    onLine->x = (x + 1) % 480;
    memcpy(zmq_msg_data(&msg), onLine, sizeof(msgBlit) + 70);
    zmq_msg_send(&msg, requester, ZMQ_SNDMORE);

    zmq_msg_init_size(&msg, sizeof(msgBlit) + 70);
    offLine->x = x % 480;
    memcpy(zmq_msg_data(&msg), offLine, sizeof(msgBlit) + 70);
    zmq_msg_send(&msg, requester, ZMQ_SNDMORE);

    zmq_msg_init_size(&msg, 0);
    zmq_msg_send(&msg, requester, 0);
    zmq_recv(requester, NULL, 0, 0);

    x = (x + 1) % 480;
  }

  zmq_close(requester);

  zmq_ctx_destroy(context);
  return 0;
}
