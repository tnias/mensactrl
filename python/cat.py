#!/usr/bin/env python2

import sys
import client

# read lines from stdin and write them to the display.
line = sys.stdin.readline().decode("utf8")
while(line != ""):
    client.writeline(line)
    line = sys.stdin.readline().decode("utf8")

