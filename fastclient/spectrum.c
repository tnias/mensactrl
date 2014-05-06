/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <fftw3.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <math.h>

#define FFTWSIZE 2048
#define MINPOS 0

#define WIDTH 480
#define HEIGHT 70

#define CMD_PIXEL 0
#define CMD_BLIT 1

#define MAXPOS (FFTWSIZE / 8)

struct msgBlit {
  uint8_t cmd;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
  volatile uint8_t data[0];
} __attribute__ ((packed));

msgBlit *screen;

void *blitScreen(void *) {
  void *context = zmq_ctx_new ();
  void *requester;

  requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect(requester, "tcp://mensadisplay:5556");
  // zmq_connect(requester, "tcp://192.168.178.147:5570");
  // zmq_connect(requester, "tcp://localhost:5556");

  while(1) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(msgBlit) + WIDTH * HEIGHT);

    memcpy(zmq_msg_data(&msg), screen, sizeof(msgBlit) + WIDTH * HEIGHT);
    zmq_msg_send(&msg, requester, ZMQ_SNDMORE);

    zmq_msg_init_size(&msg, 0);
    zmq_msg_send(&msg, requester, 0);
    zmq_recv(requester, NULL, 0, 0);
  }

  zmq_close(requester);
  zmq_ctx_destroy(context);

  return nullptr;
}

int main() {
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  screen = reinterpret_cast<msgBlit *>(malloc(sizeof(msgBlit) + WIDTH * HEIGHT));
  screen->cmd = CMD_BLIT;
  screen->x = 0;
  screen->y = 0;
  screen->w = WIDTH;
  screen->h = HEIGHT;

  pthread_t blitter;
  pthread_create(&blitter, nullptr, blitScreen, nullptr);

  fftw_complex *in, *out;
  fftw_plan plan;

  in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFTWSIZE);
  out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFTWSIZE);
  plan = fftw_plan_dft_1d(FFTWSIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, "hw:1,0",
                    SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

  /* Set period size to FFTWSIZE frames. */
  frames = FFTWSIZE;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);
  size = frames * 4; /* 2 bytes/sample, 2 channels */
  buffer = (char *) malloc(size);

//   /* We want to loop for 5 seconds */
//   snd_pcm_hw_params_get_period_time(params,
//                                          &val, &dir);
//   const long one_second = 1000000 / val;
//   const long refresh = one_second / 30;

  double minVol[MAXPOS];
  double maxVol[MAXPOS];
  for(int i = 0; i < MAXPOS; ++i) {
    minVol[i] = maxVol[i] = 0;
  }

  while (1) {
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
      continue;
    } else if (rc < 0) {
      fprintf(stderr,
              "error from read: %s\n",
              snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } else {
      for(int pos = 0; pos < FFTWSIZE; ++pos) {
        // printf("%d, %d, %d, %d\n", buffer[pos * 4], buffer[pos * 4 + 1], buffer[pos * 4 + 2], buffer[pos * 4 + 3]);
        in[pos][0] = buffer[pos * 4 + 1];
        in[pos][1] = 0;
      }

      fftw_execute(plan);

      for(int pos = 0; pos < MAXPOS; ++pos) {
        double vol = 0;
        for(int i = 0; i < 1; i++) {
          vol += fabs(out[pos+i][0]) + fabs(out[pos+i][1]);
        }

        if(vol < minVol[pos]) minVol[pos] = vol;
        if(vol > maxVol[pos]) maxVol[pos] = vol;

        int level = log(1000 * (vol - minVol[pos]) / maxVol[pos]) * HEIGHT / 6.9;

        int xStart = pos * WIDTH / MAXPOS;
        int xEnd = (pos + 1) * WIDTH / MAXPOS;
        for(int x = xStart; x < xEnd; ++x) {
          int yCross = std::min(HEIGHT, std::max(0, HEIGHT - level));

          for(int y = 0; y < yCross; ++y) {
            screen->data[y * WIDTH + x] = 0;
          }
          for(int y = yCross; y < HEIGHT; ++y) {
            screen->data[y * WIDTH + x] = 1;
          }
        }
      }
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
