#!/usr/bin/env python

import os
import sys
import types
from functools import partial

# We use two different ALSA MIDI modules.
# 1. pyalsa.alsaseq - the official bindings - weird poll implementation not
#    compatible with Qt - don't use threads as a workaround.
# 2. alsaseq - third party bindings - give us actual ALSA file descriptor
#    to block on in Qt.
# https://github.com/physacco/pyalsa/blob/master/utils/aseqdump.py
# https://github.com/bear24rw/alsa-utils/blob/master/seq/aseqdump/aseqdump.c
import alsaseq
import pyalsa.alsaseq

from PyQt4 import QtGui
from PyQt4 import QtCore

import breakslights

OCSS = """
QWidget
{
    color: black;
    background-color: #444444;
    selection-color: #2a2a2a;
    selection-background-color: #646464;
}

QTabWidget{
    border: 1px transparent black;
}

QTabWidget::pane {
    border: 1px solid #444;
    border-radius: 3px;
    padding: 3px;
}
"""

CSS = """
QWidget
{
    color: silver;
    background-color: #444444;
}

QPushButton
{
    background-color: #2a2a2a;
    border-width: 1px;
    border-color: #4A4949;
    border-style: solid;
    padding-top: 5px;
    padding-bottom: 5px;
    padding-left: 5px;
    padding-right: 5px;
    border-radius: 2px;
    outline: none;
}

QPushButton:disabled
{
    color: 444444;
    background: 333333;
}

QPushButton:checked
{
    color: black;
    background-color: #fffa87;
}

QPushButton:hover
{
    background-color: #646464;
}

QSlider::groove:vertical {
    border: 1px solid #3A3939;
    width: 8px;
    background: #201F1F;
    margin: 0 0px;
    border-radius: 2px;
}

QSlider::handle:vertical {
    background: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0.0 silver,
      stop: 0.2 #a8a8a8, stop: 1 #727272);
    border: 1px solid #3A3939;
    width: 14px;
    height: 14px;
    margin: 0 -4px;
    border-radius: 2px;
}

QWidget:focus, QMenuBar:focus
{
    border: 1px solid #78879b;
}

QTabWidget:focus, QCheckBox:focus, QRadioButton:focus, QSlider:focus
{
    border: none;
}

QTabWidget{
    /*border: 1px transparent black;*/
}

QTabWidget::pane {
    border: 1px solid #444;
    border-radius: 3px;
    padding: 3px;
}

QTabBar
{
    qproperty-drawBase: 0;
    left: 5px; /* move to the right by 5px */
}

QTabBar:focus
{
    border: 0px transparent black;
}

QTabBar::close-button  {
    image: url(:/qss_icons/rc/close.png);
    background: transparent;
}

QTabBar::close-button:hover
{
    image: url(:/qss_icons/rc/close-hover.png);
    background: transparent;
}

QTabBar::close-button:pressed {
    image: url(:/qss_icons/rc/close-pressed.png);
    background: transparent;
}

/* TOP TABS */
QTabBar::tab:top {
    color: #b1b1b1;
    border: 1px solid #4A4949;
    border-bottom: 1px transparent black;
    background-color: #302F2F;
    padding: 5px;
    border-top-left-radius: 2px;
    border-top-right-radius: 2px;
}

QTabBar::tab:top:!selected
{
    color: #b1b1b1;
    background-color: #201F1F;
    border: 1px transparent #4A4949;
    border-bottom: 1px transparent #4A4949;
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
}

QTabBar::tab:top:!selected:hover {
    background-color: #48576b;
}

"""

"""
QWidget:item:hover
{
    background-color: #272735;
    color: black;
}

QWidget:item:selected
{
    background-color: #fffa87;
}

QPushButton:focus {
    background-color: #3d8ec9;
    color: white;
}
    selection-color: #00324a;
    selection-background-color: #fffa87;
button #4d4d5a
hover #272735
selected #fffa87
text #80b2ca
seltext #00324a
border #313136

bg #595966
frameboarder #404046
"""

RING_PIXELS = 60

def error(msg):
    sys.stderr.write(msg)
    sys.stderr.write("\n")

class MIDIController(QtCore.QObject):
    """Map a MIDI controller onto Qt widgets.

    Loads a config file to map MIDI notes and continuous controllers onto a
    set of labels. These labels can then be bound onto Qt widgets using the
    bind() method."""

    def loadconfig(self, filename):
        self.config = {}
        self.labels = {}
        self.widgets = {}

        for line in open(filename):
            if line.startswith("#"):
                continue

            parts = [p for p in line.rstrip().split("\t") if len(p)]
            self.config[parts[1]] = parts[0], int(parts[2])

        for label, data in self.config.iteritems():
            self.labels[data] = label

    def ports(self):
        sequencer = pyalsa.alsaseq.Sequencer()

        for conn in sequencer.connection_list():
            if len(conn[2]) > 0:
                yield conn[0], (conn[1], conn[2][0][1])

    def connect(self, client, port):
        alsaseq.client('Breakslights', 1, 0, False)
        alsaseq.connectfrom(0, client, port)

        self.notifier = QtCore.QSocketNotifier(alsaseq.fd(),
                                               QtCore.QSocketNotifier.Read,
                                               self)

        self.notifier.activated.connect(self.inputpending)
        self.notifier.setEnabled(True)

    def disconnect(self):
        self.notifier.setEnabled(False)
        os.close(alsaseq.fd())

    def bind(self, label, widget, userData=None):
        data = self.config.get(label)

        if data is None:
            error("failed to bind unmapped: %s" % label)
            return

        self.widgets[data] = (widget, userData)

    def inputpending(self):
        while alsaseq.inputpending() > 0:
            result = alsaseq.input()

            if result[0] in (alsaseq.SND_SEQ_EVENT_NOTEON,
                             alsaseq.SND_SEQ_EVENT_NOTEOFF):

                key = "NOTE", result[7][1]
                value = result[0] == alsaseq.SND_SEQ_EVENT_NOTEON

            elif result[0] == alsaseq.SND_SEQ_EVENT_CONTROLLER:
                key = "CC", result[7][4]
                value = result[7][5]

            else:
                raise NotImplementedError

            try:
                widget, data = self.widgets[key]

            except KeyError:
                error("unbound control: %s %d - %s" % (key[0], key[1],
                                                       self.labels.get(key)))

                continue

            if isinstance(widget, QtGui.QPushButton):
                if isinstance(value, types.BooleanType):
                    self.handleQPushButton(widget, value)

                elif isinstance(value, types.IntType):
                    self.handleQPushButton(widget, value == 127)

                else:
                    raise NotImplementedError

            elif isinstance(widget, QtGui.QButtonGroup):
                if isinstance(value, types.BooleanType):
                    self.handleQButtonGroup(widget, value, data)

                elif isinstance(value, types.IntType):
                    self.handleQButtonGroup(widget, value == 127, data)

                else:
                    raise NotImplementedError

            elif isinstance(widget, QtGui.QSlider):
                if isinstance(value, types.BooleanType):
                    raise NotImplementedError

                elif not isinstance(value, types.IntType):
                    raise NotImplementedError

                self.handleQSlider(widget, value)

            else:
                raise NotImplementedError

    def handleQPushButton(self, widget, value):
        event = QtGui.QMouseEvent(
            (QtCore.QEvent.MouseButtonRelease,
             QtCore.QEvent.MouseButtonPress)[value],
            QtCore.QPoint(0, 0),
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier,
        )

        QtCore.QCoreApplication.sendEvent(widget, event)

    def handleQButtonGroup(self, widget, value, reverse):
        if not value:
            return

        buttons = widget.buttons()

        if reverse:
            buttons.reverse()

        match = None

        for button in buttons:
            if button.isChecked():
                match = button
                break

        if match is None:
            buttons[0].click()
            return

        index = buttons.index(match) + 1

        if index >= len(buttons):
            index -= len(buttons)

        buttons[index].click()

    def handleQSlider(self, widget, value):
        widget.setValue(float(value) * widget.maximum() / 127.0)

class VerticalLabel(QtGui.QLabel):
    """Custom QLabel widget with vertical text."""

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setPen(QtCore.Qt.black)
        # FIXME: adjust for height of text
        painter.translate(self.width() / 2, self.height())
        painter.rotate(-90)
        painter.drawText(0, 0, self.text())
        painter.end()

    def minimumSizeHint(self):
        size = super(VerticalLabel, self).minimumSizeHint()
        return QtCore.QSize(size.height(), size.width())

    def sizeHint(self):
        size = super(VerticalLabel, self).sizeHint()
        return QtCore.QSize(size.height(), size.width())

class SliderGrid(QtGui.QWidget):
    """Grid of sliders each with a label and a value."""

    def __init__(self, parent):
        super(SliderGrid, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)

    def update(self, label, value):
        label.setText(str(value))

    def add(self, name, minimum=None, maximum=None):
        pos = self.grid.columnCount() + 1

        label = VerticalLabel(name, self)
        self.grid.addWidget(label, 0, pos)

        slider = QtGui.QSlider(QtCore.Qt.Vertical, self)

        if minimum is not None:
            slider.setMinimum(minimum)

        if maximum is not None:
            slider.setMaximum(maximum)

        self.grid.addWidget(slider, 1, pos)

        label = QtGui.QLabel("nan", self)
        label.setAlignment(QtCore.Qt.AlignHCenter)
        self.grid.addWidget(label, 2, pos)

        slider.valueChanged.connect(partial(self.update, label))
        slider.valueChanged.emit(slider.value())

        return slider

class ConfigEditor(QtGui.QWidget):
    def __init__(self, parent, controller):
        super(ConfigEditor, self).__init__(parent)

        self.controller = controller

        box = QtGui.QVBoxLayout(self)
        combo = QtGui.QComboBox(self)
        box.addWidget(combo)
        combo.addItem("None", None)

        for name, conn in self.controller.ports():
            combo.addItem(name, conn)

        combo.currentIndexChanged.connect(partial(self.connectMIDI, combo))

    def connectMIDI(self, combo, index):
        data = combo.itemData(index).toPyObject()

        if data is None:
            self.controller.disconnect()
            return

        self.controller.connect(data[0], data[1])

class PropertyEditor(QtGui.QWidget):
    """Editor for a single animation property."""

    def __init__(self, parent, controller):
        super(PropertyEditor, self).__init__(parent)

        box = QtGui.QHBoxLayout(self)

        grid = SliderGrid(self)
        box.addWidget(grid)

        self.widgets = {}

        for k in breakslights.Property.keys:
            if type(breakslights.Property.defaults[k]) != int:
                continue

            self.widgets[k] = grid.add(breakslights.Property.names[k])
            self.widgets[k].valueChanged.connect(partial(self.changed, k))
            controller.bind("P_%s" % k, self.widgets[k])

        buttons = QtGui.QVBoxLayout()
        box.addLayout(buttons)

        for k in breakslights.Property.keys:
            if type(breakslights.Property.defaults[k]) != bool:
                continue

            button = QtGui.QPushButton(breakslights.Property.names[k], self)
            buttons.addWidget(button)
            button.setCheckable(True)
            button.clicked.connect(partial(self.changed, k))
            controller.bind("P_%s" % k, button)
            self.widgets[k] = button

    def set(self, property):
        self.property = property

        for k, w in self.widgets.iteritems():
            if not breakslights.Property.limited[k]:
                continue

            w.setMaximum(property.maximum)

        for k, w in self.widgets.iteritems():
            if type(breakslights.Property.defaults[k]) != int:
                continue

            w.setValue(self.property.values[k])

        for k, w in self.widgets.iteritems():
            if type(breakslights.Property.defaults[k]) != bool:
                continue

            w.setChecked(self.property.values[k])

    def changed(self, key, value):
        print key, value
        self.property.change(key, value)

class AnimationEditor(QtGui.QWidget):
    """Editor for a complete animation."""

    def __init__(self, parent, controller):
        super(AnimationEditor, self).__init__(parent)

        self.animation = None
        box = QtGui.QVBoxLayout(self)

        label = QtGui.QLabel("no file loaded", self)
        box.addWidget(label)

        ebox = QtGui.QHBoxLayout()
        box.addLayout(ebox)

        buttons = QtGui.QVBoxLayout()
        ebox.addLayout(buttons)

        self.widgets = {}

        button = QtGui.QPushButton("&Clear", self)
        button.setEnabled(False)
        buttons.addWidget(button)

        button = QtGui.QPushButton("&Save", self)
        button.clicked.connect(self.dump)
        buttons.addWidget(button)

        button = QtGui.QPushButton("&Mirror", self)
        button.setCheckable(True)
        button.clicked.connect(self.setmirror)
        controller.bind("A_MIRROR", button)
        buttons.addWidget(button)
        self.widgets["I"] = button

        syncs = QtGui.QHBoxLayout()
        buttons.addLayout(syncs)

        button = QtGui.QPushButton("&Start", self)
        button.clicked.connect(partial(self.sync, False))
        controller.bind("A_SYNC_START", button)
        syncs.addWidget(button)
        self.widgets["Y0"] = button

        button = QtGui.QPushButton("&End", self)
        button.clicked.connect(partial(self.sync, True))
        controller.bind("A_SYNC_END", button)
        syncs.addWidget(button)
        self.widgets["Y1"] = button

        grid = QtGui.QGridLayout()
        ebox.addLayout(grid)

        grid = SliderGrid(self)
        ebox.addWidget(grid)
        sliders = [grid.add("Segments", 1, RING_PIXELS / 2)]
        sliders[0].valueChanged.connect(self.setsegments)
        controller.bind("A_SEGMENTS", sliders[0])
        self.widgets["S"] = sliders[0]

        radios = QtGui.QVBoxLayout()
        ebox.addLayout(radios)
        self.radiogrp = QtGui.QButtonGroup(self)

        for k in breakslights.Animation.keys:
            radio = QtGui.QRadioButton(breakslights.Animation.names[k], self)
            self.radiogrp.addButton(radio)
            radios.addWidget(radio)
            radio.toggled.connect(partial(self.toggled, radio, k))

        controller.bind("A_PROPERTY_PREV", self.radiogrp, True)
        controller.bind("A_PROPERTY_NEXT", self.radiogrp, False)

        frame = QtGui.QFrame(self)
        frame.setFrameShape(QtGui.QFrame.StyledPanel)
        framebox = QtGui.QHBoxLayout(frame)
        self.editor = PropertyEditor(self, controller)
        framebox.addWidget(self.editor)
        ebox.addWidget(frame)

    def set(self, animation):
        self.animation = animation

    def toggled(self, radio, key):
        if not radio.isChecked():
            return

        self.editor.set(self.animation.ap[key])

    def changed(self, label, value):
        print "ok", value
        label.setText(str(value))

    def dump(self):
        fname = QtGui.QFileDialog.getSaveFileName(self)
        f = open(fname, "w")
        self.animation.dump(f)
        f.close()

    def setmirror(self, value):
        self.animation.setmirror(value)

    def setsegments(self, value):
        self.animation.setsegments(value)

    def sync(self, end):
        self.animation.sync(end)

class Library(QtGui.QWidget):
    """Library of available animations."""

    def __init__(self, parent, b):
        super(Library, self).__init__(parent)

        self.b = b
        box = QtGui.QHBoxLayout(self)

        tree = QtGui.QTreeView()
        box.addWidget(tree)

        tree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

        model = QtGui.QFileSystemModel()
        tree.setModel(model)

        model.setReadOnly(True)
        model.setRootPath(QtCore.QDir.currentPath())

        for c in range(1, model.columnCount()):
            tree.hideColumn(c)

        tree.setRootIndex(model.index(QtCore.QDir.currentPath()))
        tree.setDragEnabled(True)

class LiveControls(QtGui.QWidget):
    """Panel of modal (machine) controls."""

    def __init__(self, parent, machine, controller):
        super(LiveControls, self).__init__(parent)

        box = QtGui.QHBoxLayout(self)

        grid = SliderGrid(self)
        box.addWidget(grid)

        slider = grid.add("Clock", 0, 255)
        slider.valueChanged.connect(machine.divider)
        controller.bind("M_DIVIDER", slider)

        slider = grid.add("Strobe", 0, 255)
        slider.valueChanged.connect(machine.strobe)
        controller.bind("M_STROBE", slider)

        slider = grid.add("Chase", 0, 255)
        slider.valueChanged.connect(machine.chase)
        controller.bind("M_CHASE", slider)

        button = QtGui.QPushButton("Tick", self)
        button.pressed.connect(machine.tick)
        box.addWidget(button)

        button = QtGui.QPushButton("Statistics", self)
        button.clicked.connect(machine.stats)
        box.addWidget(button)

class Window(QtGui.QWidget):
    def __init__(self):
        super(Window, self).__init__()

        self.setWindowTitle('Simple')

        box = QtGui.QVBoxLayout(self)

        b = None

        if len(sys.argv) > 1:
            b = breakslights.Breakslights(sys.argv[1])

        m = breakslights.Machine(b)
        m.anims(1)
        m.rings(1)
        a = breakslights.Animation(b, 0, 60)

        self.controller = MIDIController(self)
        self.controller.loadconfig("midi.conf")

        left = QtGui.QFrame(self)
        left.setFrameShape(QtGui.QFrame.StyledPanel)
        leftbox = QtGui.QHBoxLayout(left)
        leftbox.addWidget(Library(self, b))


        tabs = QtGui.QTabWidget()
        editor = AnimationEditor(self, self.controller)
        tabs.addTab(editor, "Animation Editor")
        editor.set(a)

        config = ConfigEditor(self, self.controller)
        tabs.addTab(config, "Config")

        right = QtGui.QFrame(self)
        right.setFrameShape(QtGui.QFrame.StyledPanel)
        rightbox = QtGui.QVBoxLayout(right)
        rightbox.addWidget(tabs)
        rightbox.addWidget(LiveControls(self, m, self.controller))

        splitter = QtGui.QSplitter(QtCore.Qt.Horizontal)
        box.addWidget(splitter)
        splitter.addWidget(left)
        splitter.addWidget(right)
        box.addWidget(splitter)

        self.setStyleSheet(CSS)

def main():
    app = QtGui.QApplication(sys.argv)
    w = Window()
    w.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
