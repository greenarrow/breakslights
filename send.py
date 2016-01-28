#!/usr/bin/env python

import re
import sys
import serial

def getheader(s):
    header = s.readline().rstrip()

    if not header.startswith("breakslights"):
        sys.stderr.write("bad header: %s\n" % header)
        return False

    return True

def getresponse(s):
    while True:
        reply = s.readline().rstrip("\r\n")

        if re.match(r"^\d+$", reply) is not None:
            return int(reply)

        sys.stderr.write("recv: %s\n" % reply)

def loop(s):
    while True:
        line = sys.stdin.readline()

        if len(line) == 0:
            break

        s.write(line)
        sys.stderr.write("sent: %s\n" % line)

        if getresponse(s) != 0:
            sys.stderr.write("bad response\n")
            break

        sys.stderr.write("ok\n")

def main():
    s = serial.Serial("/dev/ttyACM0", 9600)

    if getheader(s):
        sys.stderr.write("init ok\n")
        loop(s)

    else:
        sys.stderr.write("init error\n")

    s.close()

main()
