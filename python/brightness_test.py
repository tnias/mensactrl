#!/usr/bin/env python2
import client

def bright(b, x, y, w, h):
    pixels = [b] * w * h
    client.blit(x,y,w,h,pixels)

if __name__=="__main__":
    bright(0, 0, 0, 480, 70)
    for i in range(0,16):
      bright(i*16, i*20, 0, 19, 70)
