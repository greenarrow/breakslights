#!/usr/bin/env python

import sys
from functools import partial
from PyQt4 import QtGui
from PyQt4 import QtCore
import breakslights

RING_PIXELS = 60

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

class PropertyEditor(QtGui.QWidget):
    """Editor for a single animation property."""

    def __init__(self, parent):
        super(PropertyEditor, self).__init__(parent)

        box = QtGui.QHBoxLayout(self)

        grid = SliderGrid(self)
        box.addWidget(grid)

        self.widgets = {}

        for k in breakslights.Property.keys:
            if type(breakslights.Property.defaults[k]) != int:
                continue

            self.widgets[k] = grid.add(breakslights.Property.names[k])

        buttons = QtGui.QVBoxLayout()
        box.addLayout(buttons)

        for k in breakslights.Property.keys:
            if type(breakslights.Property.defaults[k]) != bool:
                continue

            button = QtGui.QPushButton(breakslights.Property.names[k], self)
            buttons.addWidget(button)
            button.setCheckable(True)
            self.widgets[k] = button

    def set(self, property):
        self.property = property

        for k, w in self.widgets.iteritems():
            if not breakslights.Property.limited[k]:
                    continue

            w.setMaximum(property.maximum)

    def changed(self, label, converter, value):
        print "ok", value, ">", converter(value)
        label.setText(str(converter(value)))

class AnimationEditor(QtGui.QWidget):
    """Editor for a complete animation."""

    def __init__(self, parent):
        super(AnimationEditor, self).__init__(parent)

        box = QtGui.QVBoxLayout(self)

        label = QtGui.QLabel("no file loaded", self)
        box.addWidget(label)

        ebox = QtGui.QHBoxLayout()
        box.addLayout(ebox)

        buttons = QtGui.QVBoxLayout()
        ebox.addLayout(buttons)

        button = QtGui.QPushButton("&Clear", self)
        buttons.addWidget(button)

        button = QtGui.QPushButton("&Dump", self)
        buttons.addWidget(button)

        button = QtGui.QPushButton("&Mirror", self)
        button.setCheckable(True)
        buttons.addWidget(button)

        steps = QtGui.QHBoxLayout()
        buttons.addLayout(steps)

        button = QtGui.QPushButton("&<", self)
        steps.addWidget(button)

        button = QtGui.QPushButton("&>", self)
        steps.addWidget(button)

        syncs = QtGui.QHBoxLayout()
        buttons.addLayout(syncs)

        button = QtGui.QPushButton("&Start", self)
        syncs.addWidget(button)

        button = QtGui.QPushButton("&End", self)
        syncs.addWidget(button)

        grid = QtGui.QGridLayout()
        ebox.addLayout(grid)

        grid = SliderGrid(self)
        ebox.addWidget(grid)
        sliders = [grid.add("Segments", 1, RING_PIXELS / 2)]

        radios = QtGui.QVBoxLayout()

        for k in breakslights.Animation.keys:
            radio = QtGui.QRadioButton(breakslights.Animation.names[k], self)
            radios.addWidget(radio)

        ebox.addLayout(radios)

        frame = QtGui.QFrame(self)
        frame.setFrameShape(QtGui.QFrame.StyledPanel)
        framebox = QtGui.QHBoxLayout(frame)
        editor = PropertyEditor(self)
        framebox.addWidget(editor)
        ebox.addWidget(frame)

    def changed(self, label, value):
        print "ok", value
        label.setText(str(value))

class LiveControls(QtGui.QWidget):
    """"""

    def __init__(self, parent):
        super(LiveControls, self).__init__(parent)

        box = QtGui.QHBoxLayout(self)

        grid = SliderGrid(self)
        box.addWidget(grid)
        sliders = [grid.add("Clock", 0, 255)]

        button = QtGui.QPushButton("Tick", self)
        box.addWidget(button)

        button = QtGui.QPushButton("Statistics", self)
        box.addWidget(button)

    def changed(self, label, converter, value):
        print "ok", value, ">", converter(value)
        label.setText(str(converter(value)))

class Window(QtGui.QWidget):
    def __init__(self):
        super(Window, self).__init__()

        self.setWindowTitle('Simple')

        box = QtGui.QVBoxLayout(self)

        tabs = QtGui.QTabWidget()
        tabs.addTab(AnimationEditor(self), "Animation Editor")

        box.addWidget(tabs)
        box.addWidget(LiveControls(self))

def main():
    app = QtGui.QApplication(sys.argv)
    w = Window()
    w.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
