#!/usr/bin/env python2
import client
import random

if __name__=="__main__":
    for y in range(0, client.HEIGHT):
        for x in range(0, client.WIDTH):
            client.set_pixel(x,y,255)
