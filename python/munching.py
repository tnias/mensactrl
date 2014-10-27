#!/usr/bin/python
import client
import random
import time
import clearscreen

if __name__=="__main__":
    t = 0
    s = 64
    pixels = [0] * client.HEIGHT * client.WIDTH
    clearscreen.clear()
    while True:
        if t == 63:
            time.sleep(1)
        t = (t+1) % 64
        for x in range(client.WIDTH):
            for y in range(client.HEIGHT):
                #pixels[s * x + y] = 0 if (y == (x^t)) else 1
                pixels[s * x + y] = 0 if ((x&y&t)) else 255 
        client.blit(0, 0, client.WIDTH, client.HEIGHT, pixels)
        print t
        #time.sleep(0.09)

