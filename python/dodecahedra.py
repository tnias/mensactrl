#!/usr/bin/env python2
# Raoul, Hackover 2014, GPL v2
import client
import time
from numpy import sin, cos, pi, array, clip, linspace, dot
from numpy.linalg import norm
from numpy import random

CENX = client.WIDTH / 2.0
CENY = client.HEIGHT / 2.0
B = 25
tau = 0.8
sigma = 1.2
xmin = sigma * B - 2
ymin = tau * B
xmax = client.WIDTH - sigma * B
ymax = client.HEIGHT - tau * B - 2
NT = 12

def cube(a=10):
    vert = [(-a, -a, -a),
            (-a, a, -a),
            (a, a, -a),
            (a, -a, -a),
            (-a, -a, a),
            (-a, a, a),
            (a, a, a),
            (a, -a, a)]
    return array(vert)


edge_map_cube = [(0, 1),
                 (1, 2),
                 (2, 3),
                 (3, 0),
                 (4, 5),
                 (5, 6),
                 (6, 7),
                 (7, 4),
                 (0, 4),
                 (1, 5),
                 (2, 6),
                 (3, 7)]


def dode(a=15):
    p = 1.618033
    q = 1.0
    vert = [(-q, -q, -q), # 0
            (-q, q, -q),
            (q, q, -q),
            (q, -q, -q),
            (-q, -q, q), # 4
            (-q, q, q),
            (q, q, q),
            (q, -q, q),
            (0, 1/p, p), # 8
            (0, -1/p, p),
            (0, 1/p, -p),
            (0, -1/p, -p),
            (p, 0, 1/p), # 12
            (-p, 0, 1/p),
            (p, 0, -1/p),
            (-p, 0, -1/p),
            (1/p, p, 0), # 16
            (-1/p, p, 0),
            (1/p, -p, 0),
            (-1/p, -p, 0)]
    #
    #          5---------6
    #         /    8    /
    #        /    9    /
    #       4---------7
    #    11              12
    #          17   16
    #    15  19   18     14
    #          1---------2
    #       Z /   10    /
    #       |Y   11    /
    #       0-X-------3
    #
    return a*array(vert)


edge_map_dode = [(3, 14),
                 (2, 14),
                 (6, 12),
                 (7, 12),
                 (12, 14),
                 (0, 15),
                 (1, 15),
                 (4, 13),
                 (5, 13),
                 (13, 15),
                 (1,10),
                 (2,10),
                 (0, 11),
                 (3, 11),
                 (10,11),
                 (4, 9),
                 (7, 9),
                 (5, 8),
                 (6, 8),
                 (9, 8),
                 (4, 19),
                 (0, 19),
                 (5, 17),
                 (1, 17),
                 (6, 16),
                 (2, 16),
                 (7, 18),
                 (3, 18),
                 (19, 18),
                 (17, 16)]


def rotatex(vertl, phi = pi/3):
    Rx = array([[1,         0,        0],
                [0,  cos(phi), sin(phi)],
                [0, -sin(phi), cos(phi)]])
    return dot(Rx, vertl.T).T


def rotatey(vertl, phi = pi/3):
    Ry = array([[ cos(phi), 0, sin(phi)],
                [        0, 1,        0],
                [-sin(phi), 0, cos(phi)]])
    return dot(Ry, vertl.T).T


def rotatez(vertl, phi = pi/3):
    Rz = array([[ cos(phi), sin(phi), 0],
                [-sin(phi), cos(phi), 0],
                [        0,        0, 1]])
    return dot(Rz, vertl.T).T


def propagate(x, v):
    ti = x[:] + v[:]
    x[:] = ti
    if abs(ti[0]) < xmin or abs(ti[0]) > xmax:
        v[0] *= -1
    if abs(ti[1]) < ymin or abs(ti[1]) > ymax:
        v[1] *= -1


def move(vertl, center):
    return [center + v for v in vertl]


def project(vertlist):
    vert_xy = []
    for v in vertlist:
        u = v[:2]
        u[1] += 10
        u[1] *= tau
        vert_xy.append(u)
    return vert_xy


def reflect(v1, v2, n):
    v1n = v1 - 2*dot(v1, n) * n
    v2n = v2 - 2*dot(v2, n) * n
    return v1n, v2n


def line(u, v):
    xd = v[0] - u[0]
    yd = v[1] - u[1]
    t = linspace(0, 1, NT)
    lx = u[0] + xd*t
    ly = u[1] + yd*t
    return lx.round(), ly.round()


def show(vl, el):
    pixels = []
    pixelsold = []
    for e in el:
        lx, ly = line(vl[e[0]], vl[e[1]])
        for lxi, lyi in zip(lx, ly):
            pixels.append( (lxi, lyi, 255) )
            pixelsold.append( (lxi, lyi, 0) )

    return pixels, pixelsold


def clear():
    pixels = [0] * client.HEIGHT * client.WIDTH
    client.blit(0, 0, client.WIDTH, client.HEIGHT, pixels)


if __name__=="__main__":
    x1 = array([CENX-100, CENY, 0])
    v1 = array([1.5, 0.75, 0])
    x2 = array([CENX+100, CENY, 0])
    v2 = array([-2.5, -0.25, 0])

    c1 = dode(16)
    c2 = dode(16)
    m = edge_map_dode

    offset = len(c1)
    m += [(mi[0]+offset, mi[1]+offset) for mi in m]

    c1 = rotatez(c1, pi/4.0)
    c1 = rotatex(c1, pi/3.0)
    c2 = rotatez(c2, pi/4.0)
    c2 = rotatex(c2, pi/3.0)

    r1y = pi/50
    r1z = pi/30
    r2y = -pi/30
    r2z = -pi/50

    ti = 0
    pixold = []
    clear()
    while(True):
        c1 = rotatey(c1, r1y)
        c1 = rotatez(c1, r1z)
        c2 = rotatey(c2, r2y)
        c2 = rotatez(c2, r2z)

        propagate(x1, v1)
        propagate(x2, v2)

        n = x1 - x2
        if norm(n) < 54:
            v1, v2 = reflect(v1, v2, n/norm(n))
            ry = clip(2*random.uniform(), -1.5, 1.5)
            rz = clip(2*random.uniform(), -1.5, 1.5)
            r1y = clip(r1y*ry, -0.28, 0.28)
            r1z = clip(r1z*rz, -0.28, 0.28)
            r2y = clip(r2y*ry, -0.28, 0.28)
            r2z = clip(r2z*rz, -0.28, 0.28)

        cc1 = project(move(c1, x1))
        cc2 = project(move(c2, x2))

        pix, pixoldn = show(cc1+cc2, m)
        client.set_pixels(pixold+pix)
        pixold = pixoldn

        ti += 1
        if ti > 2000:
            ti = 0
            pixold = []
            x1 = array([CENX-100, CENY, 0])
            v1 = array([1.5, 0.75, 0])
            x2 = array([CENX+100, CENY, 0])
            v2 = array([-2.5, -0.25, 0])
            clear()

        time.sleep(0.01)
