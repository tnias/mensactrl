#!/usr/bin/env python

import datetime
import time
import client

while True:
    t = datetime.datetime.now().strftime(" %H:%M:%S")
    client.write(client.NUM_SEG_X - len(t), 0, t);
    time.sleep(1)

