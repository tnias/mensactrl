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

struct mensa_fb {
	int x_res, y_res;
	int fd;
	uint8_t *inputfb;
	uint16_t *fbmem;
};

void encodeToFb(struct mensa_fb *mensafb) {
	int pix, row, idx;
	uint16_t pattern;

	for (row = 0; row < mensafb->y_res; row++) {
		for (pix=0; pix<mensafb->x_res; pix++) {
			idx = row*mensafb->x_res + pix;
			if (mensafb->inputfb[idx] > 0)
				pattern = 1;
			else
				pattern = 0;
			pattern |= row << 13;

			mensafb->fbmem[idx] = pattern;
		}
	}
}


void setPixel(struct mensa_fb *mensafb, int col, int row, uint8_t bright)
{
	int idx = row * mensafb->x_res + col;

	if (col < 0 || col >= mensafb->x_res)
		return;
	if (row < 0 || row >= mensafb->y_res)
		return;

	mensafb->inputfb[idx] = bright;
}

static struct mensa_fb *setup_fb(const char *devname, int x, int y)
{
	struct mensa_fb *mensafb = malloc(sizeof(struct mensa_fb));
	int i;

	mensafb->fd = open(devname, O_RDWR);
	if (mensafb->fd < 0) {
		perror("opening fb");
		free(mensafb);
		exit(1);
	}
	mensafb->inputfb = malloc(x*y*3);
	if (!mensafb->inputfb) {
		free(mensafb);
		exit(1);
	}
	memset(mensafb->inputfb, 0, x*y*3);

	mensafb->fbmem=mmap(NULL, x*5*24*2, PROT_READ|PROT_WRITE, MAP_SHARED, mensafb->fd, 0);
	if (mensafb->fbmem==NULL) {
		perror("mmap'ing fb");
		free(mensafb->inputfb);
		free(mensafb);
		exit(1);
	}
	for (i = 0; i < x*y*5*24; i++) {
		/* Init frame buffer bit clock. */
		if (i % 5 == 0)
			mensafb->fbmem[i] = 0xffff;
		if (i % 5 == 4)
			mensafb->fbmem[i] = 0x00;
	}

	mensafb->x_res = x;
	mensafb->y_res = y;

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

	mensafb = setup_fb(argv[1], 40, 7);

	while (1) {
		struct pixel pix;
		zmq_recv(subscriber, &pix, sizeof(pix), 0);
		printf("(%u, %u): bright=%02x\n", pix.x, pix.y,  pix.bright);
		setPixel(mensafb, pix.x, pix.y, pix.bright);
		encodeToFb(mensafb);
	}

	return 0;
}
