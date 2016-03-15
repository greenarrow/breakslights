#!/usr/bin/env python

import re
import sys
import serial

class Breakslights(object):
    def __init__(self, port):
        self.serial = serial.Serial(port, 57600, rtscts=True, timeout=4)

        header = self.serial.readline().rstrip()

        if not header.startswith("breakslights"):
            self.close()
            raise RuntimeError("bad header '%s'" % header)

    def close(self):
        self.serial.close()

    def _getresponse(self):
        while True:
            reply = self.serial.readline().rstrip("\r\n")

            if re.match(r"^\d+$", reply) is not None:
                return int(reply)

            sys.stderr.write("recv: %s\n" % reply)

    def send(self, line):
        self.serial.write(line)
        sys.stderr.write("sent: %s\n" % line)

        if self._getresponse() != 0:
            raise RuntimeError("bad response")

def main():
    retries = 3

    while True:
        try:
            b = Breakslights("/dev/ttyUSB0")

        except RuntimeError:
            if retries > 0:
                retries -= 1
                continue

            raise

        break

    while True:
        line = sys.stdin.readline()
        retries = 3

        if len(line) == 0:
            break

        while True:
            try:
                b.send(line)
                break

            except RuntimeError:
                if retries > 0:
                    retries -= 1
                    continue

                raise

        sys.stderr.write("ok\n")

if __name__ == "__main__":
    main()
