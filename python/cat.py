#!/usr/bin/env python

import sys
import client

# read lines from stdin and write them to the display.
line = sys.stdin.readline()
while(line != ""):
    client.writeline(line)
    line = sys.stdin.readline()

