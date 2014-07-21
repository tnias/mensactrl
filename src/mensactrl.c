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
#include <string.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>

#include <zmq.h>

#include "common.h"

#define ROWS_PER_LINE 7
#define LINES_PER_MODULE 2
#define COLS_PER_MODULE 40

struct mensa_fb {
	int x_res, y_res;
	int hmodules, vmodules;
	int fd;
	uint8_t *inputfb;
	uint16_t *fbmem;
	ssize_t size;
};

int verbose = 0;

static int is_inbounds(struct mensa_fb *mensafb, int x, int y, int width, int height)
{
    if (y < 0 || height < 0 || y + height > mensafb->y_res ||
		    x < 0 || width < 0 || x + width > mensafb->x_res)
	    return 0;
    return 1;
}

static int blit_area(struct mensa_fb *mensafb, const int col, const int row,
		const int width, const int height)
{
    int r, c;
    int vmpos, vpos, hmpos, hpos, pos;

    if (!is_inbounds(mensafb, col, row, width, height))
	    return -1;

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


	    if (mensafb->inputfb[c + r * mensafb->x_res] == 0) {
		/* clear bit */
		mensafb->fbmem[pos] &= ~(1<<(mensafb->vmodules - 1 - vmpos));
	    } else {
		/* set bit */
		mensafb->fbmem[pos] |= (1<<(mensafb->vmodules - 1 - vmpos));
	    }
	}
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
	mensafb->inputfb = malloc(mensafb->x_res * mensafb->y_res);
	if (!mensafb->inputfb) {
		free(mensafb);
		exit(1);
	}
	memset(mensafb->inputfb, 0, mensafb->x_res * mensafb->y_res);

	mensafb->fbmem=mmap(NULL, mensafb->size * 2 + mensafb->x_res * LINES_PER_MODULE * 2,
			PROT_READ|PROT_WRITE, MAP_SHARED, mensafb->fd, 0);
	if (mensafb->fbmem==NULL) {
		perror("mmap'ing fb");
		free(mensafb->inputfb);
		free(mensafb);
		exit(1);
	}
	for (i = 0; i < mensafb->size + mensafb->x_res * LINES_PER_MODULE; i++)
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

int main(int argc, char *argv[]) {
	struct mensa_fb *mensafb;
	void *context = zmq_ctx_new ();
	void *responder = zmq_socket (context, ZMQ_REP);
	int rc;

	if (argc != 2)
		exit(1);

	rc = zmq_bind (responder, "tcp://*:5556");
	if (rc < 0)
		perror("zmq_bind");

	mensafb = setup_fb(argv[1], 12, 5);

	while (1) {
                while (1) {
			zmq_msg_t message;
			zmq_msg_init (&message);
			zmq_msg_recv (&message, responder, 0);

			if(zmq_msg_size(&message)) {
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
