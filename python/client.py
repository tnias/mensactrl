#!/usr/bin/python

PWIDTH = 5
WIDTH = PWIDTH*96
PHEIGHT = 7
PPAD = 5
HEIGHT = PHEIGHT*10

import sys

SERVER = "tcp://localhost:5571"

if len(sys.argv) >= 2:
  SERVER = sys.argv[1]

import struct
import zmq

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect(SERVER)

def set_pixel(x, y, v):
  tx = struct.pack('<BiiB', 0, x, y, v)
  socket.send_multipart([tx, b''])
  rx = socket.recv()
  #print("Received reply %s [%r]" % ((x, y, v), rx))

def set_pixels(pixels):
  msg = []
  for x, y, v in pixels:
    msg.append(struct.pack('<BiiB', 0, x, y, v))
  socket.send_multipart(msg + [b''])
  rx = socket.recv()

