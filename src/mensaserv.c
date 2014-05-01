/*
 * Control the Mensadisplay connected to the LCDIF
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

/*
 * Layout:
 *
 *  Connector on the Individual boards
 *  ______
 *  | 1  2|
 *  |_3  4|
 *  |5  6|
 *  |~7  8|
 *  | 9 10|
 *  ~~~~~~
 * 
 *  Pinout
 *  Display		iMX233	Function
 * 
 * 1:	NC
 * 2:	SIN/OUT		LCD0-4	Data in for the shift regs
 * 3:	SCK		LCDCLK	Clock (rising edge)
 * 4:	GND		GND
 * 5:	A0		LCD13	Address for the line to drive (LSB)
 * 6:	A1		LCD14	Possible values: 0-6 for lines 1-7
 * 7:	A2		LCD15	Address for line MSB
 * 8:	/G		GND	Global Display /Enable
 * 9:	VCC		+5V
 * 10:	RCK		EN	Strobe to latch data into the output
 * 
 * 1: Keep /G low
 * 2: Write data via SIN und SCK into the shift regs
 * 3: Select row through A0-A2
 * 4: Update LED outputs with RCK
 * 5: Goto 2
 */

static void fade(void *publisher)
{
	int i = 0;
	struct pixel pix;
	while (1) {
		pix.x = i / 7;
		pix.y = i % 7;
		pix.bright = 0;

		zmq_send(publisher, &pix, sizeof(pix), 0);
		i = (i + 1) % (7*20);
		pix.bright = 255;
		zmq_send(publisher, &pix, sizeof(pix), 0);
		printf("(%u, %u): bright=%02x\n", pix.x, pix.y, pix.bright);
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
