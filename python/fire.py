#!/usr/bin/env python2
import client
import random
import math

""" Fire effect """

cool_offs = 0
cool_map = [0] * client.HEIGHT * client.WIDTH
for x in range(1, client.WIDTH-2):
    for y in range(1, client.HEIGHT-2):
	    cool_map[y*client.WIDTH + x] = random.random() * 3 + math.sin((x + random.random()*10)/480.0*math.pi*7) * math.sin((y + random.random() * 10)/70.0*math.pi*5) * 3


def init_fire():
    current = [0] * client.HEIGHT * client.WIDTH
    return current

# Smooth image and cool (darken) pixels according to cool_map
def avg_cooled(x, y, buf):
	res = 0
	res += buf[(y*client.WIDTH) + x-1]
	res += buf[(y*client.WIDTH) + x+1]
	res += buf[(y*client.WIDTH-1) + x]
	res += buf[(y*client.WIDTH+1) + x]
	res += buf[(y*client.WIDTH) + x]
	res = int(res / 5.0 - cool_map[(y+ cool_offs)%client.HEIGHT*client.WIDTH + x])
	if res < 0:
		res = 0
	return res

# Move everything up one pixel and generate new fire at the bottom
def move_and_smooth(current, new):
    global cool_offs
    for x in range(1, client.WIDTH-2):
        for y in range(1, client.HEIGHT-2):
		new[y*client.WIDTH + x] = avg_cooled(x, y+1, current)
    for i in range(5, client.WIDTH-5, 3):
        bright = int(random.random() * 180 + 70)
	new[(client.HEIGHT-1) * client.WIDTH + i] = bright
	new[(client.HEIGHT-1) * client.WIDTH + i + 1] = bright
	new[(client.HEIGHT-1) * client.WIDTH + i + 2] = bright
        new[(client.HEIGHT-2) * client.WIDTH + i] = bright
        new[(client.HEIGHT-2) * client.WIDTH + i + 1] = bright
        new[(client.HEIGHT-2) * client.WIDTH + i + 2] = bright
    # The cool map moves as well
    cool_offs = (cool_offs - 1) % client.HEIGHT

if __name__=="__main__":
    current = init_fire()
    new = [0] * client.HEIGHT * client.WIDTH
    sending = [(x*x)/256 for x in current]
    while (True):
        client.blit(0, 0, client.WIDTH, client.HEIGHT, sending, rec = False)
	move_and_smooth(current, new)
	current = new
	# x^2 will work better with the brightness levels we have
        sending = [(x*x)/256 for x in current]
	client.rec()
