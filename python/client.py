#!/usr/bin/python

import os
import bitmapfont

SPLIT_N = 1
SPLIT_I = 0

SPLIT = os.environ.get("SPLIT")
if SPLIT:
  SPLIT = map(int, SPLIT.split('/'))
  SPLIT_N = SPLIT[1]
  SPLIT_I = SPLIT[0]-1

NUM_SEG_X = 96
NUM_SEG_Y = 10
PWIDTH = 5
WIDTH = PWIDTH*NUM_SEG_X/SPLIT_N
WOFFSET = WIDTH*SPLIT_I
PHEIGHT = 7
PPAD = 5
HEIGHT = PHEIGHT*NUM_SEG_Y

import sys

SERVER = "tcp://localhost:5570"

if len(sys.argv) >= 2:
  SERVER = sys.argv[1]

import struct
import zmq

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect(SERVER)

################################################################################
# RAW PIXEL HANDLING
################################################################################

def set_pixel(x, y, v):
  x += WOFFSET
  tx = struct.pack('<BiiB', 0, x, y, v)
  socket.send_multipart([tx, b''])
  rx = socket.recv()
  #print("Received reply %s [%r]" % ((x, y, v), rx))

def set_pixels(pixels):
  msg = []
  for x, y, v in pixels:
    x += WOFFSET
    msg.append(struct.pack('<BiiB', 0, x, y, v))
  socket.send_multipart(msg + [b''])
  rx = socket.recv()

def blit(x, y, w, h, pixels):
  x += WOFFSET
  assert w*h == len(pixels)
  msg = struct.pack('<Biiii', 1, x, y, w, h)+b''.join(map(chr, pixels))
  socket.send_multipart([msg, b''])
  rx = socket.recv()

################################################################################
# TEXT RENDERING WITH BITMAP FONT
################################################################################

# return array of size PWIDTH * PHEIGHT (indexed by row, then column)
def char_to_pixel_segment(c):
    pixels = [0] * PWIDTH * PHEIGHT
    if(c in bitmapfont.FONT.keys()):
        for x in xrange(0, PWIDTH):
            for y in xrange(0, PHEIGHT):
                pix = (bitmapfont.FONT[c][x] & (1<<y)) >> y
                pixels[y * PWIDTH + x] = pix
    return pixels

# write string, starting at segment x,y. no boundary checks are done, text may
# be clipped at the border, in this case False is returned.
def write(x, y, string):
    for c in string:
        blit(x*PWIDTH, y*PHEIGHT, PWIDTH, PHEIGHT, char_to_pixel_segment(c))
        x += 1
        if(x > NUM_SEG_X):
            return False

# write text at beginning of line and clear remaining horizontal space
def writeline(y, string):
    write(0, y, string)
    write(len(string), y, ' ' * (NUM_SEG_X-len(string)))
