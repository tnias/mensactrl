/*
 * Control Mensadisplay connected to the LCDIF
 *
 * (C) 2014 Daniel Willmann <daniel@totalueberwachung.de>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>

#include <zmq.h>

#include "common.h"
#include "fb.h"

#define ROWS_PER_LINE 7
#define LINES_PER_MODULE 2
#define COLS_PER_MODULE 40
#define BRIGHT_LEVELS 17

#define POWER_TIMEOUT 180000 /* milliseconds */
#define POWER_GPIO_PATH "/sys/class/gpio/gpio91/value"

#define BENCH_LOOPS 100

struct mensa_fb {
	int x_res, y_res;
	int hmodules, vmodules;
	int fd;
	uint8_t *inputfb;
	uint16_t *fbmem;
	ssize_t size;
};

int verbose = 0;

int power = 0;

static int is_inbounds(struct mensa_fb *mensafb, int x, int y, int width, int height)
{
    if (y < 0 || height < 0 || y + height > mensafb->y_res ||
		    x < 0 || width < 0 || x + width > mensafb->x_res)
	    return 0;
    return 1;
}

static int blit_fullscreen(struct mensa_fb *mensafb)
{
    int r, c, b, i, l;
    int pos, offs, row_addr;
    uint8_t thresh;
    uint16_t val;
    uint16_t *fbline;

    offs = 0;
    fbline = mensafb->fbmem;
    for (b = 0; b < BRIGHT_LEVELS - 1; b++) {
      thresh = 255 * (b + 1) / BRIGHT_LEVELS;
      for (l = 0; l < mensafb->hmodules * LINES_PER_MODULE * ROWS_PER_LINE; l++) {
	r = l / (mensafb->hmodules * LINES_PER_MODULE) + ((l & 1) ? 0 : ROWS_PER_LINE);
	c = mensafb->x_res - 1 - ((l % (mensafb->hmodules * LINES_PER_MODULE)) / LINES_PER_MODULE * COLS_PER_MODULE);
	row_addr = ((6 + (offs + l * COLS_PER_MODULE) / (mensafb->x_res * LINES_PER_MODULE)) % ROWS_PER_LINE) << 5;
	for (pos = 0; pos < COLS_PER_MODULE; pos++, c--) {
	  val = row_addr;
	  for (i = 0; i < mensafb->vmodules; i++) {
	    if (mensafb->inputfb[c + (r + LINES_PER_MODULE * ROWS_PER_LINE * i) * mensafb->x_res] >= thresh)
	      val |= (1 << (mensafb->vmodules - 1 - i));
	  }
	  fbline[pos] = val;
	}
	fbline += COLS_PER_MODULE;
      }
      offs += LINES_PER_MODULE * ROWS_PER_LINE * COLS_PER_MODULE * mensafb->hmodules;
    }
    return 0;
}

static int blit_area(struct mensa_fb *mensafb, const int col, const int row,
		const int width, const int height)
{
    int r, c, b;
    int vmpos, vpos, hmpos, hpos, pos;
    int offs;
    uint8_t thresh;

    if (col == 0 && mensafb->x_res == width &&
        row == 0 && mensafb->y_res == height)
	    return blit_fullscreen(mensafb);

    if (!is_inbounds(mensafb, col, row, width, height))
	    return -1;

    offs = 0;
    for (b = 0; b < BRIGHT_LEVELS-1; b++) {
      thresh = 255 * (b+1) / BRIGHT_LEVELS;
      for (r = row; r < row + height; r++) {
        for (c = col; c < col + width; c++) {
          /* Calculate module and position inside the module */
          vmpos = r/(LINES_PER_MODULE*ROWS_PER_LINE);
	  vpos = r%(LINES_PER_MODULE*ROWS_PER_LINE);
	  hmpos = c/(COLS_PER_MODULE);
	  hpos = c%(COLS_PER_MODULE);

	  pos = hpos + hmpos*COLS_PER_MODULE*LINES_PER_MODULE;

	  if (vpos >= ROWS_PER_LINE) {
            vpos -= ROWS_PER_LINE;
	    pos += COLS_PER_MODULE;
	  }

	  /* Invert order */
	  pos = (LINES_PER_MODULE * COLS_PER_MODULE * mensafb->hmodules - 1) - pos;

	  /* Add in row offset */
	  pos = pos + vpos*(mensafb->hmodules*COLS_PER_MODULE*LINES_PER_MODULE);

          if (mensafb->inputfb[c + r * mensafb->x_res] < thresh) {
            /* clear bit */
            mensafb->fbmem[offs + pos] &= ~(1<<(mensafb->vmodules - 1 - vmpos));
          } else {
            /* set bit */
            mensafb->fbmem[offs + pos] |= (1<<(mensafb->vmodules - 1 - vmpos));
          }
	}
      }
      offs += LINES_PER_MODULE * ROWS_PER_LINE * COLS_PER_MODULE * mensafb->hmodules;
    }
    return 0;
}

void setPixel(struct mensa_fb *mensafb, int col, int row, uint8_t bright)
{
	int idx = row * mensafb->x_res + col;

	if (!is_inbounds(mensafb, col, row, 1, 1))
	    return;

	mensafb->inputfb[idx] = bright;
}

static struct mensa_fb *setup_fb(const char *devname, int hmodules, int vmodules)
{
	struct mensa_fb *mensafb = malloc(sizeof(struct mensa_fb));
	struct fb_var_screeninfo vsinfo;
	int i;

	mensafb->x_res = COLS_PER_MODULE * hmodules;
	mensafb->y_res = ROWS_PER_LINE * LINES_PER_MODULE * vmodules;
	mensafb->size = mensafb->x_res * ROWS_PER_LINE * LINES_PER_MODULE;
	mensafb->fd = open(devname, O_RDWR);

	if (mensafb->fd < 0) {
		perror("opening fb");
		free(mensafb);
		exit(1);
	}

	if (ioctl(mensafb->fd, FBIOGET_VSCREENINFO, &vsinfo)) {
		perror("could not get var screen info, ignoring");
	}

	vsinfo.xres = 960;
	vsinfo.xres_virtual = vsinfo.xres;
	vsinfo.yres = 7*(BRIGHT_LEVELS-1) + 1;
	vsinfo.yres_virtual = vsinfo.yres;

	vsinfo.pixclock = 125000;

	if (ioctl(mensafb->fd, FBIOPUT_VSCREENINFO, &vsinfo)) {
		perror("could not set var screen info, ignoring");
	}

	mensafb->inputfb = malloc(mensafb->x_res * mensafb->y_res);
	if (!mensafb->inputfb) {
		free(mensafb);
		exit(1);
	}
	memset(mensafb->inputfb, 0, mensafb->x_res * mensafb->y_res);

	mensafb->fbmem=mmap(NULL, mensafb->size * 2 * (BRIGHT_LEVELS-1) + mensafb->x_res * LINES_PER_MODULE * 2,
			PROT_READ|PROT_WRITE, MAP_SHARED, mensafb->fd, 0);
	if (mensafb->fbmem==NULL) {
		perror("mmap'ing fb");
		free(mensafb->inputfb);
		free(mensafb);
		exit(1);
	}
	memset(mensafb->fbmem, 0, mensafb->size * 2 * (BRIGHT_LEVELS-1) + mensafb->x_res * LINES_PER_MODULE * 2);
	for (i = 0; i < mensafb->size * (BRIGHT_LEVELS-1) + mensafb->x_res * LINES_PER_MODULE; i++)
		mensafb->fbmem[i] = ((6 + i / (mensafb->x_res * LINES_PER_MODULE)) % 7)<<5;

	mensafb->hmodules = hmodules;
	mensafb->vmodules = vmodules;

	return mensafb;
}

int copyArea(struct mensa_fb *mensafb, int x, int y, int w, int h, uint8_t *data, size_t len)
{
	int row;
	if (!is_inbounds(mensafb, x, y, w, h)) {
		if (verbose)
			printf("Geometry overflow: %ix%i+%i+%i\n", x, y, w, h);
		return -1;
	}

	if (len != (size_t)w*h) {
		if (verbose)
			printf("Blit size mismatch: want %i, have %zi\n", w*h, len);
		return -1;
	}

	for (row = 0; row < h; row++) {
		int idx = (row + y) * mensafb->x_res + x;
		memcpy(&mensafb->inputfb[idx], &data[row*w], w);
	}
	return 0;
}

int handlePixel(struct mensa_fb *mensafb, struct pixel *pix, size_t len) {
	if (len != sizeof(struct pixel))
		return -1;

	if (!is_inbounds(mensafb, pix->x, pix->y, 1, 1)) {
		return -1;
	}

	setPixel(mensafb, pix->x, pix->y, pix->bright);
	return blit_area(mensafb, pix->x, pix->y, 1, 1);
}

int handleBlit(struct mensa_fb *mensafb, struct blit *blit, size_t len)
{
	int rc;

	if (len < sizeof(struct blit))
		return -1;

	if (!is_inbounds(mensafb, blit->x, blit->y, blit->width, blit->height)) {
		return -1;
	}

	rc = copyArea(mensafb, blit->x, blit->y, blit->width, blit->height, blit->data, len - sizeof(struct blit));
	if (rc < 0)
		return rc;

	return blit_area(mensafb, blit->x, blit->y, blit->width, blit->height);
}

int handleFill(struct mensa_fb *mensafb, struct fill *fill, size_t len) {
	int row;

	if (len != sizeof(struct fill))
		return -1;

	if (!is_inbounds(mensafb, fill->x, fill->y, fill->width, fill->height)) {
		return -1;
	}

	for (row = fill->y; row < fill->y + fill->height; row++) {
		int idx = row * mensafb->x_res + fill->x;
		memset(&mensafb->inputfb[idx], fill->bright, fill->width);
	}

	return blit_area(mensafb, fill->x, fill->y, fill->width, fill->height);
}

void handleCommand(struct mensa_fb *mensafb, struct packet *p, size_t len) {
	int rc = 0;
	switch(p->cmd) {
		case CMD_PIXEL: rc = handlePixel(mensafb, &p->pixel, len - 1); break;
		case CMD_BLIT: rc = handleBlit(mensafb, &p->blit, len - 1); break;
		case CMD_FILL: rc = handleFill(mensafb, &p->fill, len - 1); break;
	default:
	       if (verbose)
		       printf("Unknown command: 0x%02x\n", p->cmd);
	}
	if ((rc < 0) && (verbose))
		printf("Error handling command 0x%02x\n", p->cmd);
}

int setPower(int enabled) {
	int fd, ret;

	if (power == enabled)
	    return 0;

	fd = open(POWER_GPIO_PATH, O_WRONLY);
	if  (fd < 0) {
		perror("open power gpio");
		return fd;
	}

	ret = write(fd, enabled ? "1" : "0", 1);
	if (ret != 1) {
		perror("Could not write to fd");
		return ret;
	}


	power = enabled;

	close(fd);
	return 0;
}

void runBenchmark(struct mensa_fb *mensafb) {
	clock_t t = clock();
	for (int i = 0; i < BENCH_LOOPS; i++) {
		blit_area(mensafb, 0, 0, mensafb->x_res, mensafb->y_res);
	}
	t = clock() - t;
	printf("%fus / blit_area\n", (float)t/100.0);
}

int main(int argc, char *argv[]) {
	struct mensa_fb *mensafb;
	void *context = zmq_ctx_new ();
	void *responder = zmq_socket (context, ZMQ_REP);
	int rc, timeout = POWER_TIMEOUT;
	int opt;
	bool benchmark = false;

	while ((opt = getopt(argc, argv, "b")) != -1) {
		switch (opt) {
		case 'b':
			benchmark = true;
			break;
		default:
			fprintf(stderr, "Usage: %s [-b] framebuffer\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "missing framebuffer argument\n");
		exit(EXIT_FAILURE);
	}

	mensafb = setup_fb(argv[optind], 12, 5);

	if (benchmark) {
		runBenchmark(mensafb);
		exit(0);
	}

	rc = zmq_setsockopt (responder, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
	if (rc < 0)
		perror("zmq_setsockopt");

	rc = zmq_bind (responder, "tcp://*:5556");
	if (rc < 0)
		perror("zmq_bind");

	while (1) {
                while (1) {
			zmq_msg_t message;
			zmq_msg_init (&message);
			rc = zmq_msg_recv (&message, responder, 0);
			if (rc < 0 && errno == EAGAIN)
				setPower(0);

			if(zmq_msg_size(&message)) {
				setPower(1);
				handleCommand(mensafb, (struct packet *)zmq_msg_data(&message), zmq_msg_size(&message));
			}

			if (!zmq_msg_more(&message)) {
				zmq_msg_close(&message);
				break;
			}

			zmq_msg_close(&message);
		}

		zmq_send(responder, NULL, 0, 0);
	}

	return 0;
}
