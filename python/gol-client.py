#!/usr/bin/python

import client

SIZE = 1

import time

def rect(x,y,w,h,r,g,b):
    for i in xrange(x,x+w):
        for j in xrange(y,y+h):
            #pixel(i,j,r,g,b)
            client.set_pixel(i, j, r)

def draw(x, y, v):
    rect(x*SIZE, y*SIZE, SIZE, SIZE, 255*v, 255*v, 255*v)

import random

class Game(object):
    def __init__(self, state, infinite_board = True):
        self.state = state
        self.width = state.width
        self.height = state.height
        self.infinite_board = infinite_board

    def step(self, count = 1):
        for generation in range(count):
            new_board = [[False] * self.width for row in range(self.height)]

            for y, row in enumerate(self.state.board):
                for x, cell in enumerate(row):
                    neighbours = self.neighbours(x, y)
                    previous_state = self.state.board[y][x]
                    should_live = neighbours == 3 or (neighbours == 2 and previous_state == True)
                    new_board[y][x] = should_live

            self.state.update(new_board)

    def neighbours(self, x, y):
        count = 0

        for hor in [-1, 0, 1]:
            for ver in [-1, 0, 1]:
                if not hor == ver == 0 and (self.infinite_board == True or (0 <= x + hor < self.width and 0 <= y + ver < self.height)):
                    count += self.state.board[(y + ver) % self.height][(x + hor) % self.width]

        return count

    def display(self):
        return self.state.display()

class State(object):
    def __init__(self, positions, x, y, width, height):
        self.width = width
        self.height = height
        self.board = [[False] * self.width for row in range(self.height)]

        active_cells = []

        for l, row in enumerate(positions.splitlines()):
            for r, cell in enumerate(row.strip()):
                if cell == 'o':
                    active_cells.append((r,l))

        board = [[False] * width for row in range(height)]

        for cell in active_cells:
            board[cell[1] + y][cell[0] + x] = True

        self.update(board)

    def update(self, new):
        for y, row in enumerate(new):
            for x, cell in enumerate(row):
                if self.board[y][x] != cell:
                    self.board[y][x] = cell
                    draw(x, y, cell)

    def display(self):
        output = ''

        for y, row in enumerate(self.board):
            for x, cell in enumerate(row):
                if self.board[y][x]:
                    output += 'o'
                else:
                    output += '.'
            output += '\n'

        return output

glider = """
oo.
o.o
o..
"""

gun = """
........................o...........
......................o.o...........
............oo......oo............oo
...........o...o....oo............oo
oo........o.....o...oo..............
oo........o...o.oo....o.o...........
..........o.....o.......o...........
...........o...o....................
............oo......................
"""

my_game = Game(State(gun,
    x = 20, y = 10,
    width = client.WIDTH/SIZE, height = client.HEIGHT/SIZE)
)

while True:
    #print my_game.display()
    my_game.step(1)
