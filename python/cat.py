#!/usr/bin/env python

import sys
import client

# read lines from stdin and write them to the display.
# includes scrolling.
# FIXME: scrolling is slow. use a pixel buffer and blitting.
if __name__ == "__main__":
    y = 0
    lines = []
    line = sys.stdin.readline()
    while(line != ""):
        client.write(0, y, line)
        lines.append(line)
        if(y == client.NUM_SEG_Y):
            lines.reverse()
            lines.pop()
            lines.reverse()
            y = 0
            for l in lines:
                client.write(0, y, l)
                y += 1
        else:
            y += 1
        line = sys.stdin.readline()

