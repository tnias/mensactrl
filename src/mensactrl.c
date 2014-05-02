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

static void blit_area(struct mensa_fb *mensafb, const int col, const int row,
		const int width, const int height)
{
    int r, c;
    int vmpos, vpos, hmpos, hpos, pos;

    if (row < 0 || height < 0 || row + height > mensafb->y_res ||
		    col < 0 || width < 0 || col + width > mensafb->x_res)
	    return;

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

            pos = pos + vpos*(mensafb->vmodules*COLS_PER_MODULE*LINES_PER_MODULE);

	    if (mensafb->inputfb[c + r * mensafb->x_res] == 0) {
                /* clear bit */
                mensafb->fbmem[pos] &= ~(1<<(mensafb->vmodules - vmpos));
            } else {
                /* set bit */
                mensafb->fbmem[pos] |= (1<<(mensafb->vmodules - vmpos));
            }
        }
    }
}


void encodeToFb(struct mensa_fb *mensafb)
{
	blit_area(mensafb, 0, 0, mensafb->x_res, mensafb->y_res);
}
//	int pix, row, cols, idx, fbidx, i, ledrow;
//	uint16_t pattern;
//
//	for (fbidx = 0; fbidx < mensafb->x_res * mensafb->y_res / mensafb->modules; fbidx++) {
//		cols = mensafb->x_res * 2;
//
//		row = fbidx / cols;
//		ledrow = row;
//		pix = fbidx % cols;
//
//		if (pix / 40 % 2 == 0) {
//			pix = pix + (pix / 40 * 40);
//		} else {
//			pix = pix + (pix / 40 * 40) + 40;
//			row = row + 7;
//		}
//
//
//		pattern = 0;
//		for (i = 0; i < mensafb->modules; i++) {
//			idx = row * mensafb->x_res + pix + i * mensafb->x_res * 14;
//			if (mensafb->inputfb[idx] > 0)
//				pattern |= 1 << i;
//		}
//			/* During update of the next line we want to display the
//			 * content of the last one */
//			pattern |= (ledrow-1 % 7) << 5;
//
//			mensafb->fbmem[fbidx] = pattern;
//	}
//}


void setPixel(struct mensa_fb *mensafb, int col, int row, uint8_t bright)
{
	int idx = row * mensafb->x_res + col;

	if (col < 0 || col >= mensafb->x_res)
		return;
	if (row < 0 || row >= mensafb->y_res)
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

	mensafb->fbmem=mmap(NULL, mensafb->size * 2,
			PROT_READ|PROT_WRITE, MAP_SHARED, mensafb->fd, 0);
	if (mensafb->fbmem==NULL) {
		perror("mmap'ing fb");
		free(mensafb->inputfb);
		free(mensafb);
		exit(1);
	}
	for (i = 0; i < mensafb->size ; i++)
		mensafb->fbmem[i] = (i / (mensafb->x_res * LINES_PER_MODULE))<<5;

	mensafb->hmodules = hmodules;
	mensafb->vmodules = vmodules;

	return mensafb;
}

int main(int argc, char *argv[]) {
	struct mensa_fb *mensafb;
	void *context = zmq_ctx_new ();
	void *subscriber = zmq_socket (context, ZMQ_SUB);
	int rc;

	if (argc != 2)
		exit(1);

	rc = zmq_connect (subscriber, "tcp://10.0.0.1:5556");
	if (rc < 0)
		perror("zmq_connect");

	rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,
			NULL, 0);
	if (rc < 0)
		perror("zmq_connect");

	mensafb = setup_fb(argv[1], 12, 5);

	while (1) {
		struct pixel pix;
		zmq_recv(subscriber, &pix, sizeof(pix), 0);
		printf("(%u, %u): bright=%02x\n", pix.x, pix.y,  pix.bright);
		setPixel(mensafb, pix.x, pix.y, pix.bright);
		encodeToFb(mensafb);
	}

	return 0;
}
