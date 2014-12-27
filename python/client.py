#!/usr/bin/env python2
# -*- coding: utf-8 -*-

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

SERVER = "tcp://151.217.19.24:5556"

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

def blit(x, y, w, h, pixels, rec=True):
  x += WOFFSET
  assert w*h == len(pixels)
  msg = struct.pack('<Biiii', 1, x, y, w, h)+b''.join(map(chr, pixels))
  socket.send_multipart([msg, b''])
  if rec:
    rx = socket.recv()

def rec():
  rx = socket.recv()

################################################################################
# TEXT RENDERING WITH BITMAP FONT
################################################################################

# screen buffer used for text handling
print WIDTH
print HEIGHT
SCREENBUFFER = [0] * WIDTH * HEIGHT

# blit to screen buffer, also updates screen
def screenbuf_blit(x, y, w, h, pixels):
    blit(x, y, w, h, pixels)
    assert w*h == len(pixels)
    for dy in range(0, h):
        for dx in range(0, w):
            if(y+dy >= HEIGHT or x+dx >= WIDTH):
                return
            pix = pixels[dy * w + dx];
            SCREENBUFFER[(y+dy) * WIDTH + (x+dx)] = pix

# scroll the screen buffer up y pixels, also updates screen
def screenbuf_scroll(y):
    buf = SCREENBUFFER
    for dy in range(y, HEIGHT):
        for x in range(0, WIDTH):
            SCREENBUFFER[(dy-y) * WIDTH + x] = buf[dy * WIDTH + x]
    screenbuf_render()

# draw screen buffer to screen
def screenbuf_render():
    blit(0, 0, WIDTH, HEIGHT, SCREENBUFFER)

# return array of size PWIDTH * PHEIGHT (indexed by row, then column)
def char_to_pixel_segment(c):
    pixels = [0] * PWIDTH * PHEIGHT

    if(c not in bitmapfont.FONT.keys()):
        c = u"☐";

    for x in xrange(0, PWIDTH):
        for y in xrange(0, PHEIGHT):
            pix = (bitmapfont.FONT[c][x] & (1<<y)) >> y
            pixels[y * PWIDTH + x] = pix * 255
    return pixels

# write string, starting at segment x,y. Tabs are expanded to 8 spaces, new
# lines always begin at the given x position. No boundary checks are done, text
# may be clipped at the border, in this case the function returns.
# This function returns a tuple (x,y,success) where (x,y) gives the position of
# the last character written, and success is set to False if the function
# returned because of clipped text.
def write(x, y, string):
    orig_x = x
    string = string.replace("\t", " "*8)
    for c in string:
        if c == "\n":
            y += 1
            x = orig_x
        if ord(c) < 0x1f:
            pass
        else:
            pixels = char_to_pixel_segment(c)
            screenbuf_blit(x*PWIDTH, y*PHEIGHT, PWIDTH, PHEIGHT, pixels)
            x += 1

        if(x > NUM_SEG_X):
            return (x,y,False)

    return (x,y,True)

# write line to screen as if on a terminal, scroll up if neccessary
cur_line = 0
def writeline(string):
    global cur_line
    if(cur_line >= NUM_SEG_Y):
        scrollline()
        cur_line -= 1
    (new_x, new_y, success) = write(0, cur_line, string.strip("\r\n"))
    cur_line = new_y

    # clear remaining row
    clear_chars = (NUM_SEG_X-new_x)
    screenbuf_blit(new_x * PWIDTH, cur_line * PHEIGHT,
        clear_chars * PWIDTH, PHEIGHT,
        [0] * clear_chars * PWIDTH * PHEIGHT)

    cur_line += 1

# scroll the content y lines up and clear last line
def scrollline(y = 1):
    screenbuf_scroll(y*PHEIGHT)
    screenbuf_blit(0, (NUM_SEG_Y-1) * PHEIGHT, WIDTH, PHEIGHT,
        [0] * WIDTH * PHEIGHT)
