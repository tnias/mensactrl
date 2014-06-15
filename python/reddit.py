#!/usr/bin/env python2
# 10x48x5x7 Pixel

import sys, client, praw, pygame
from time import sleep

SERVER = "tcp://localhost:5570"
XOFF = 0
YOFF = 0
TEXT = "Franz jagt im komplett verwahrlosten Taxi quer durch Bayern"

pygame.init()

def clear():
    pixels = [0] * client.HEIGHT * client.WIDTH
    client.blit(0,0,client.WIDTH,client.HEIGHT,pixels)

def render(TEXT,XOFF=0,YOFF=0):
    font = pygame.font.Font("/usr/share/fonts/misc/5x7.pcf.gz", 7)
    text = font.render(TEXT, True, (255, 255, 255), (0, 0, 0))
    pxarray = pygame.PixelArray(text)
    pixels = []
    for x in range(text.get_width()):
        for y in range(text.get_height()):
            pixels.append((XOFF+x, YOFF+y, pxarray[x][y]))
    del pxarray
    client.set_pixels(pixels)

r = praw.Reddit(user_agent='my_cool_application')
reddit = ["opensource"
         ,"linux"
         ,"netsec"
         ,"sysadmin"
         ,"worldnews"
         ,"shittyaskscience"
         ,"showerthoughts"
         ,"explainlikeimcalvin"
         ,"nosleep"
         ,"pettyrevenge"
         ,"all"]

while True:
  for i in reddit:
    clear()
    render("reddit.com/r/" + i,0,0)
    YOFF = 7
    submissions = r.get_subreddit(i).get_hot(limit=10)
    subs = [str(x) for x in submissions]
    for i in range(1,10):
      votes,title = subs[i].split(' :: ',1)
      TEXT = '%s :: %s' % (votes.rjust(5),title)
      render(TEXT,0,YOFF)
      YOFF += 7
    sleep(30)
