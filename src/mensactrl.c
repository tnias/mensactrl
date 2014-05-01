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

struct ws281x_fb {
	int x_res, y_res;
	int fd;
	uint8_t *inputfb;
	uint16_t *fbmem;
};

//This will encode a framebuffer with per-led RGB values into the weird framebuffer
//data values we need to form the correct WS2811 driving waveform.
void encodeToFb(struct ws281x_fb *wsfb) {
	int pix, col, bit, bitmask, strip, n;
	int p=0;
	for (pix=0; pix<wsfb->x_res; pix++) { //STRIPLEN points...
		for (col=0; col<3; col++) { //...of 3 bytes (r, g. b)
			bitmask=0x80;
			for (bit=0; bit<8; bit++) { //...of 8 bits each.
				/* At 4MHz, a bit has 5 cycles. For an 1, it's high 4, low one. For an 0, it's high one, low 4. */
				p++; /* First cycle is always 1 */
				n=0;
				//Iterate through every LED-strip to fetch the bit it needs to send out. Combine those
				//in n.
				for (strip=0; strip<wsfb->y_res; strip++) {
					if (wsfb->inputfb[(wsfb->x_res*strip*3)+(pix*3)+col]&bitmask) n|=1<<strip;
				}
				wsfb->fbmem[p++]=n;
				wsfb->fbmem[p++]=n;	//Middle 3 are dependent on bit value
				wsfb->fbmem[p++]=n;
				p++; /* Last cycle is always 0 */
				bitmask>>=1; //next bit in byte
			}
		}
	}
}


//Helper function to set the value of a single pixel in the 'framebuffer'
//of LED pixel values.
void setPixel(struct ws281x_fb *wsfb, int pixel, int strip, int r, int g, int b) {
	int pos=(strip*wsfb->x_res*3)+pixel*3;
	if (strip<0 || strip>=wsfb->x_res) return;
	if (pixel<0 || pixel>=wsfb->y_res) return;
	wsfb->inputfb[pos++]=r; //My strips have R and G switched.
	wsfb->inputfb[pos++]=g;
	wsfb->inputfb[pos++]=b;
}

static struct ws281x_fb *setup_fb(const char *devname, int x, int y)
{
	struct ws281x_fb *wsfb = malloc(sizeof(struct ws281x_fb));
	int i;

	wsfb->fd = open(devname, O_RDWR);
	if (wsfb->fd < 0) {
		perror("opening fb");
		free(wsfb);
		exit(1);
	}
	wsfb->inputfb = malloc(x*y*3);
	if (!wsfb->inputfb) {
		free(wsfb);
		exit(1);
	}
	memset(wsfb->inputfb, 0, x*y*3);

	wsfb->fbmem=mmap(NULL, x*5*24*2, PROT_READ|PROT_WRITE, MAP_SHARED, wsfb->fd, 0);
	if (wsfb->fbmem==NULL) {
		perror("mmap'ing fb");
		free(wsfb->inputfb);
		free(wsfb);
		exit(1);
	}
	for (i = 0; i < x*y*5*24; i++) {
		/* Init frame buffer bit clock. */
		if (i % 5 == 0)
			wsfb->fbmem[i] = 0xffff;
		if (i % 5 == 4)
			wsfb->fbmem[i] = 0x00;
	}

	wsfb->x_res = x;
	wsfb->y_res = y;

	return wsfb;
}

int main(int argc, char *argv[]) {
	struct ws281x_fb *wsfb;
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

	wsfb = setup_fb(argv[1], 10, 2);

	while (1) {
		struct pixel pix;
		zmq_recv(subscriber, &pix, sizeof(pix), 0);
		printf("(%u, %u): r=%02x g=%02x b=%02x\n", pix.x, pix.y,  pix.r, pix.g, pix.b);
		setPixel(wsfb, pix.x, pix.y, pix.r, pix.g, pix.b);
		encodeToFb(wsfb);
	}

	return 0;
}
