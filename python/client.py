#!/usr/bin/python

import os

SPLIT_N = 1
SPLIT_I = 0

SPLIT = os.environ.get("SPLIT")
if SPLIT:
  SPLIT = map(int, SPLIT.split('/'))
  SPLIT_N = SPLIT[1]
  SPLIT_I = SPLIT[0]-1

PWIDTH = 5
WIDTH = PWIDTH*96/SPLIT_N
WOFFSET = WIDTH*SPLIT_I
PHEIGHT = 7
PPAD = 5
HEIGHT = PHEIGHT*10

import sys

SERVER = "tcp://localhost:5570"

if len(sys.argv) >= 2:
  SERVER = sys.argv[1]

import struct
import zmq

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect(SERVER)

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

