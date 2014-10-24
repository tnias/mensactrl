#!/usr/bin/env python2

import sys, client

SERVER = "tcp://localhost:5570"
XOFF = 0
YOFF = 0
TEXT = "Hello World!"

if __name__=="__main__":
  if len(sys.argv) >= 4:
    XOFF = int(sys.argv[2])
    YOFF = int(sys.argv[3])
  if len(sys.argv) >= 5:
    TEXT = sys.argv[4]

import pygame
pygame.init()

font = pygame.font.Font("/usr/share/fonts/X11/misc/5x7.pcf.gz", 7)
text = font.render(TEXT, True, (255, 255, 255), (0, 0, 0))
pxarray = pygame.PixelArray(text)
pixels = []
for x in range(text.get_width()):
    for y in range(text.get_height()):
        pixels.append((XOFF+x, YOFF+y, pxarray[x][y]))
del pxarray
client.set_pixels(pixels)

