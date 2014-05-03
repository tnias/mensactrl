#!/usr/bin/python

PWIDTH = 5
WIDTH = PWIDTH*96
PHEIGHT = 7
PPAD = 5
HEIGHT = (PHEIGHT+PPAD)*9+PHEIGHT
ZOOM = 2

import pygame, sys
pygame.init()

size = (WIDTH*ZOOM, HEIGHT*ZOOM)
screen = pygame.display.set_mode(size)

import struct
import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5570")

def set_pixel(x, y, v):
    v = v/255.0
    row = y / 7
    if row in [0, 1, 2, 9]:
        c = (0xff*v, 0, 0)
    elif row in [3, 4, 5]:
        c = (0, 0xff*v, 0)
    else:
        c = (0xff*v, 0xa5*v, 0)
    y += y / PHEIGHT * PPAD
    screen.fill(c, (x*ZOOM, y*ZOOM, ZOOM, ZOOM))

while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

    if not socket.poll(1):
        pygame.display.update()
        continue

    messages = socket.recv_multipart()
    #print("New multimessage")
    for message in messages:
        if not message:
            break
        cmd, = struct.unpack_from('<B', message)
        #print repr(message)
        if cmd == 0: # set pixel
            cmd, x, y, v = struct.unpack_from('<BiiB', message)
            #print("Received set pixel: %r, %r, %r, %r" % (cmd, x, y, v))
            set_pixel(x, y, v)
        elif cmd == 1: # blit
            cmd, x, y, w, h = struct.unpack_from('<Biiii', message)
            #print("Received blit: %r, %r, %r, %r, %r" % (cmd, x, y, w, h))
            for r in range(h):
                for c in range(w):
                    set_pixel(x+c, y+r, ord(message[17+r*w+c]))
        else:
            cmd, = struct.unpack_from('<B', message)
            print("Received unknown: %r" % (cmd,))
    socket.send(b'')
