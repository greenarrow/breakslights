#!/usr/bin/env python

import re
import sys
import serial

def tohex(value):
    return (hex(int(value))[2:]).zfill(2)

class Breakslights(object):
    def __init__(self, port):
        self.serial = serial.Serial(port, 57600, rtscts=True, timeout=4)

        # reset device
        self.serial.setDTR(True)
        self.serial.flushInput()
        self.serial.setDTR(False)

        header = self.serial.readline().rstrip()

        if not header.startswith("breakslights"):
            self.close()
            raise RuntimeError("bad header '%s'" % header)

    def close(self):
        self.serial.close()

    def _getresponse(self):
        while True:
            reply = self.serial.readline().rstrip("\r\n")
            assert len(reply)

            if re.match(r"^\d+$", reply) is not None:
                return int(reply)

            sys.stderr.write("recv: %s\n" % reply)

    def send(self, line):
        self.serial.write(line)
        sys.stderr.write("sent: %s\n" % line)

        if self._getresponse() != 0:
            raise RuntimeError("bad response")

class Property(object):
    #   code  name          default type    log     limited
    properties = (
        ("C", "Constant",   0,      int,    False,  True),
        ("D", "Divider",    0,      int,    True,   False),
        ("S", "Step",       1,      int,    False,  True),
        ("B", "Bounce",     False,  bool,   False,  False),
        ("V", "Reverse",    False,  bool,   False,  False),
        ("N", "Min",        0,      int,    False,  True),
        ("X", "Max",        0,      int,    False,  True),
    )

    keys = [p[0] for p in properties]
    names = dict((p[0], p[1]) for p in properties)
    defaults = dict((p[0], p[2]) for p in properties)
    types = dict((p[0], p[3]) for p in properties)
    logscales = dict((p[0], p[4]) for p in properties)
    limited = dict((p[0], p[5]) for p in properties)

    def __init__(self, output, anim, code, maximum, default):
        self.output = output
        self.anim = anim
        self.code = code
        self.maximum = maximum

        self.values = dict(self.defaults)
        self.values["C"] = default

    def change(self, code, value):
        if self.types[code] == int:
            val = tohex(value)

        elif self.types[code] == bool:
            if value:
                val = "1"
            else:
                val = "0"
        else:
            assert False

        self.values[code] = value

        cmd = "A%s %s%s%s\n" % (tohex(self.anim), self.code, code, val)
        sys.stdout.write(cmd)

        if self.output is not None:
            self.output.send(cmd)

    def dump(self, stream=sys.stdout):
        for k in self.keys:
            if self.values[k] == self.defaults[k]:
                continue

            if self.types[k] == int:
                val = tohex(self.values[k])

            elif self.types[k] == bool:
                if self.values[k]:
                    val = "1"
                else:
                    val = "0"
            else:
                assert False

            cmd = "A%s %s%s%s\n" % (tohex(self.anim), self.code, k, val)
            stream.write(cmd)

class Animation(object):
    #   code  name              range   default
    properties = (
        ("F", "Fill",           None,   None),
        ("O", "Rotation",       None,   0),
        ("T", "Offset",         None,   0),
        ("H", "Hue",            240,    0),
        ("B", "Lightness",      255,    255 / 4),
        ("U", "Hue 2",          240,    0),
        ("G", "Lightness 2",    255,    0),
    )

    keys = [p[0] for p in properties]
    names = dict((p[0], p[1]) for p in properties)
    maximums = dict((p[0], p[2]) for p in properties)
    defaults = dict((p[0], p[3]) for p in properties)

    def __init__(self, output, index, pixels):
        self.output = output
        self.index = index
        self.pixels = pixels
        self.ap = {}

        for k in self.keys:
            maximum = self.maximums[k]

            if maximum is None:
                maximum = pixels

            default = self.defaults[k]

            if default is None:
                default = pixels

            self.ap[k] = Property(output, index, k, maximum, default)

        self.segments = 1
        self.mirror = False

    def setsegments(self, value):
        cmd = "A%s S%s\n" % (tohex(self.index), tohex(value))
        sys.stdout.write(cmd)
        self.segments = value

        if self.output is not None:
            self.output.send(cmd)

    def setmirror(self, value):
        self.mirror = value
        cmd = "A%s I%s\n" % (tohex(self.index), "01"[bool(value)])
        sys.stdout.write(cmd)

        if self.output is not None:
            self.output.send(cmd)

    def sync(self, end=False):
        if self.output is None:
            return

        if end:
            self.output.send("A00 Y1\n")
        else:
            self.output.send("A00 Y0\n")

    def dump(self, stream=sys.stdout):
        stream.write("MA01\n")
        stream.write("MR01\n")
        stream.write("A00 L\n")

        cmd = "A%s S%s\n" % (tohex(self.index), tohex(self.segments))
        stream.write(cmd)

        cmd = "A%s I%s\n" % (tohex(self.index), "01"[bool(self.mirror)])
        stream.write(cmd)

        for p in self.ap.itervalues():
            p.dump(stream)

class Machine(object):
    def __init__(self, output):
        self.output = output

    def rings(self, value):
        if self.output is None:
            return

        self.output.send("MR%s\n" % tohex(value))

    def anims(self, value):
        if self.output is None:
            return

        self.output.send("MA%s\n" % tohex(value))

    def tick(self):
        if self.output is None:
            return

        self.output.send("MT\n")

    def divider(self, value):
        if self.output is None:
            return

        self.output.send("MD%s\n" % tohex(value))

    def stats(self):
        if self.output is None:
            return

        self.output.send("MI\n")

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
