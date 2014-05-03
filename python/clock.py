#!/usr/bin/python

import font_client

from datetime import datetime

import dateutil.tz

while True:
  #font_client.draw(datetime.now().isoformat(), points=30)
  font_client.draw(datetime.now(dateutil.tz.tzlocal()).isoformat(), points=25)

