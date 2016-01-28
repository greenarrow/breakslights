#!/usr/bin/python

import sys
import gtk
import breakslights

def connect():
    retries = 3

    while True:
        try:
            b = breakslights.Breakslights("/dev/ttyACM0")

        except RuntimeError:
            if retries > 0:
                retries -= 1
                continue

            raise

        return b

def send(b, cmd):
    try:
        b.send(cmd)

    except RuntimeError:
        sys.stderr.write("error: %s\n" % cmd)

def slide(widget, prefix, b):
    cmd = "%s%s\n" % (prefix, str(int(widget.get_value())).zfill(3))
    send(b, cmd)

def filled(widget, b, value):
    cmd = "A000 I%s\n" % value
    send(b, cmd)

def enter(widget, b):
    cmd = "%s\n" % widget.get_text()
    widget.set_text("")
    send(b, cmd)

def slider(b, label, prefix, upper):
    s = gtk.VScale()

    s.set_range(0, upper)
    s.set_increments(1, 16)
    s.set_digits(0)
    s.set_inverted(True)
    s.set_size_request(35, 160)
    s.connect("value-changed", slide, prefix, b)

    l = gtk.Label(label)

    box = gtk.VBox(spacing=5)
    box.add(l)
    box.add(s)

    return box

def editor(b):
    e = gtk.HBox(spacing=5)

    switches = gtk.VBox(spacing=5)
    e.add(switches)

    radio = None

    for c, l in (("N", "none"), ("F", "fill"), ("O", "offset"),
                 ("R", "rotation")):

        radio = gtk.RadioButton(radio, l)
        radio.connect("toggled", filled, b, c)
        switches.add(radio)

    e.add(slider(b, "strobe", "MS", 255))
    e.add(slider(b, "speed", "A000 D", 255))
    e.add(slider(b, "fill", "A000 L", 60))
    e.add(slider(b, "segments", "A000 S", 60))
    e.add(slider(b, "offset", "A000 O", 60))
    e.add(slider(b, "rotation", "A000 N", 60))
    e.add(slider(b, "step", "A000 P", 60))

    return e

def main():
    b = connect()

    send(b, "MA001\n")
    send(b, "MR001\n")
    send(b, "A000 F001100\n")

    win = gtk.Window()
    box = gtk.VBox(spacing=5)
    win.add(box)

    box.add(editor(b))

    manual = gtk.Entry()
    manual.connect("activate", enter, b)
    box.add(manual)

    win.connect("delete-event", gtk.main_quit)
    win.show_all()
    gtk.main()

if __name__ == "__main__":
    main()
