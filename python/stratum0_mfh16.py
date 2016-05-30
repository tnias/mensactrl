#!/usr/bin/env python2

import font_client

from datetime import datetime
from time import sleep

import dateutil.tz

while True:
  #font_client.draw(datetime.now().isoformat(), points=30)
  for i in range(0,62,2):
    font_client.draw("Stratum0.org", points=i)
  sleep(2)
  for i in range(0,60,6):
    font_client.draw("Stratum0.org", points=60-i)
  sleep(.1)
  font_client.draw("#mfh16", points=60)
  sleep(1.2)
