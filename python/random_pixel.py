#!/usr/bin/env python2
import client
import random
import time

blockx = 5
blocky = 7

if __name__=="__main__":
    while(True):
        x = random.random() * (client.WIDTH/blockx)
        y = random.random() * (client.HEIGHT/blocky)
	x = int(x) * blockx
	y = int(y) * blocky
	if random.random() > 0.5:
            bright = random.random()
	    bright = (bright * bright) * 255
	    pix = [int(bright)] * blockx * blocky
	    client.blit(x, y, blockx, blocky, pix)
	else:
	    pix = [0] * blockx * blocky
	    client.blit(x, y, blockx, blocky, pix)

