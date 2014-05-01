/*
 * Control WS281X LEDs connected to the LCDIF
 *
 * Code taken from the proof-of-concept tool by
 * (C) 2013 Jeroen Domburg (jeroen AT spritesmods.com)

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

struct pixel {
	uint16_t x, y;
	uint8_t r, g, b;
} __attribute__((packed));

static void fade(void *publisher)
{
	struct pixel pix1 = { .x = 0, .y = 0, .r = 0, .g = 0, .b = 0 };
	struct pixel pix2 = { .x = 1, .y = 0, .r = 0, .g = 0, .b = 0 };
	struct pixel pix3 = { .x = 2, .y = 0, .r = 0, .g = 0, .b = 0 };
	int state = 0;
	while (1) {
		switch (state) {
		case 0:
			pix1.r++;
			pix2.g++;
			pix3.b++;
			if (pix1.r == 255)
				state++;
			break;
		case 1:
			pix1.r--;
			pix1.g++;
			pix2.g--;
			pix2.b++;
			pix3.b--;
			pix3.r++;
			if (pix1.g == 255)
				state++;
			break;
		case 2:
			pix1.g--;
			pix1.b++;
			pix2.b--;
			pix2.r++;
			pix3.r--;
			pix3.g++;
			if (pix1.b == 255)
				state++;
			break;
		case 3:
			pix1.b--;
			pix1.r++;
			pix2.r--;
			pix2.g++;
			pix3.g--;
			pix3.b++;
			if (pix1.r == 255)
				state = 1;
			break;
		}
		zmq_send(publisher, &pix1, sizeof(pix1), 0);
		printf("(%u, %u): r=%02x g=%02x b=%02x\n", pix1.x, pix1.y, pix1.r, pix1.g, pix1.b);
		//zmq_send(publisher, &pix2, sizeof(pix2), 0);
		//printf("(%u, %u): r=%02x g=%02x b=%02x\n", pix2.x, pix2.y, pix2.r, pix2.g, pix2.b);
		//zmq_send(publisher, &pix3, sizeof(pix3), 0);
		//printf("(%u, %u): r=%02x g=%02x b=%02x\n", pix3.x, pix3.y, pix3.r, pix3.g, pix3.b);
		usleep(10000);
	}
}


int main(void)
{
	void *context = zmq_ctx_new ();
	void *publisher = zmq_socket (context, ZMQ_PUB);
	int rc;

	rc = zmq_bind (publisher, "tcp://*:5556");
	if (rc < 0)
		perror("zmq_bind");


	fade(publisher);

	return 0;
}
