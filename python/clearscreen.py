#!/usr/bin/env python2
import client
import random

def clear():
    pixels = [0] * client.HEIGHT * client.WIDTH
    client.blit(0,0,client.WIDTH,client.HEIGHT,pixels)

if __name__=="__main__":
    clear()
