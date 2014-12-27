#!/usr/bin/env python2

import font_client

from datetime import datetime
from time import sleep

import dateutil.tz

while True:
  #font_client.draw(datetime.now().isoformat(), points=30)
  font_client.draw("Stratum0.org", points=60)
  sleep(2)
  font_client.draw("GOTO =>  ", points=60)
  sleep(2)
