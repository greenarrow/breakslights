#!/usr/bin/env python

import sys
import math

def output(rings):
    print '<svg>'

    r = 30.0

    for ia, addr in enumerate(sorted(rings.keys())):
        theta = 360.0 / len(rings[addr])

        for i, value in enumerate(rings[addr]):
            x = r * math.sin(math.radians(i * theta + 0.5 * theta))
            y = r * math.cos(math.radians(i * theta + 0.5 * theta))

            x += r * 2
            y += r * 2

            x += r * 3 * ia

            print '<circle cx="%d" cy="%d" r="5" stroke="black" ' \
                'fill="#%s" />' % (x, y, value)

    print '</svg>'
    print '<br />'

if __name__ == "__main__":
    stream = sys.stdin
    rings = {}
    dirty = {}

    print '<html><body>'

    while True:
        line = stream.readline()

        if len(line) == 0:
            break

        parts = line.strip().split()
        addr = int(parts[0], 16)

        if addr in dirty:
            rings.update(dirty)
            output(rings)
            dirty = {}

        dirty[addr] = parts[1:]

    print '</body></html>'
