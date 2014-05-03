#!/usr/bin/python

import sys, client

import pygame
pygame.init()

screen = pygame.Surface((client.WIDTH, client.HEIGHT), depth=8)

def send(screen):
    w = screen.get_width()
    h = screen.get_height()
    pxarray = pygame.PixelArray(screen)
    pixels = []
    for y in range(h):
        for x in range(w):
            pixels.append(pxarray[x][y])
    del pxarray
    client.blit(0, 0, w, h, pixels)

prev_mx = None
font = None

def draw(text, mark=None, points=None):
    global prev_mx, font
    points = points if points else 35 if mark else 60
    font = pygame.font.SysFont("DejaVu Sans Mono", points, bold=True)
    count = len(text)
    text = font.render(text, True, (255, 255, 255), (0, 0, 0))
    screen.set_palette(text.get_palette())
    screen.fill((0, 0, 0))
    w = text.get_width()
    h = text.get_height()
    x = (client.WIDTH-w)/2
    y = (client.HEIGHT-h)/2
    if mark:
        cw = w/count
        mx = x+cw*(mark+0.5)
        if not mx == prev_mx:
            for size in range(240, 5, -15):
                pygame.draw.line(screen, 255,
                        (mx, 0), 
                        (mx, client.HEIGHT-1),
                        size)
                send(screen)
                pygame.draw.line(screen, 0,
                        (mx, 0), 
                        (mx, client.HEIGHT-1),
                        size)
            prev_mx = mx
        pygame.draw.line(screen, 255,
                (mx, 0), 
                (mx, client.HEIGHT-1),
                5)
    screen.blit(text, (x, y))

    send(screen)

if __name__=="__main__":
    if len(sys.argv) >= 3:
        text = sys.argv[2].decode('utf-8')
    else:
        text = "Hello World!"
    if len(sys.argv) >= 4:
        mark = int(sys.argv[3])
    else:
        mark = None

    draw(text, mark)
