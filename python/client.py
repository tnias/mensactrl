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
  tx = bytearray(9)
  struct.pack_into('iiB', tx, 0, x, y, v)
  socket.send(tx)
  rx = socket.recv()
  #print("Received reply %s [%r]" % ((x, y, v), rx))

