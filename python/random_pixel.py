#!/usr/bin/env python2
import client
import random

if __name__=="__main__":
    while(True):
        x = random.random() * client.WIDTH
        y = random.random() * client.HEIGHT
        client.set_pixel(x,y, 1 if random.random() > 0.5 else 0)

