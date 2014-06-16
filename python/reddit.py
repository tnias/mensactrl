#!/usr/bin/env python2
# 10x48x5x7 Pixel

import sys, client, praw
from time import sleep

SERVER = "tcp://localhost:5570"
XOFF = 0
YOFF = 0
TEXT = ""

r = praw.Reddit(user_agent='Stratum0 Braunschweig Mensadisplay Parser')
reddit = ["opensource"
         ,"linux"
         ,"netsec"
         ,"sysadmin"
         ,"worldnews"
         ,"shittyaskscience"
         ,"showerthoughts"
         ,"explainlikeimcalvin"
         ,"pettyrevenge"
         ,"all"]

while True:
  for i in reddit:
    client.writeline("reddit.com/r/" + i)
    submissions = r.get_subreddit(i).get_hot(limit=9)
    subs = [str(x) for x in submissions]
    for i in range(0,9):
      votes,title = subs[i].split(' :: ',1)
      TEXT = '%s :: %s' % (votes.rjust(5),title)
      client.writeline(TEXT)
    sleep(30)
