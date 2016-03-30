-include .config

VALGRIND = valgrind -q --error-exitcode=1 --leak-check=yes \
	--undef-value-errors=yes --show-reachable=yes --log-file=/dev/null

OBJCOPY ?= avr-objcopy
SIZE ?= avr-size
AVRDUDE ?= avrdude

BAUD ?= 57600

DEVICE ?= atmega328p
CLOCK ?= 16000000

PROGRAMMER ?= -c usbtiny

CFLAGS += -Wall
CFLAGS += -MMD -MP

ADFLAGS += $(PROGRAMMER)
ADFLAGS += -p $(DEVICE)

ifdef DEBUG
	CPPFLAGS += -D DEBUG=1
	CFLAGS += -g
endif

CPPFLAGS += -DBAUD=$(BAUD)
CPPFLAGS += -DPOSIX=1

# This allows us to do AVR and POSIX builds at he same time by isolating
# the relocatables.
%.avr.o:	CPPFLAGS += -UPOSIX
%.avr.o %.elf:	CC = avr-gcc
%.avr.o %.elf:	CFLAGS += -W -O -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)

UNIT_TESTS = unit/hue

REGRESSION_TESTS = tests/fills.lc tests/strobe.lc tests/chase.lc \
	tests/mirror.lc tests/chopflash.lc \
	tests/ripple.lc tests/shadows.lc tests/multiprop.lc \
	tests/offset.lc tests/limits.lc

TARGETS = breakslights breakslights.elf breakslights.hex

.PHONY:		all test clean pylint flash

%.unit:		%
	$< | diff -u -- $<.stdout -
	$(VALGRIND) -- $< > /dev/null

%.reg:		% breakslights
	./breakslights < $< | diff -u -- $<.stdout -
	$(VALGRIND) -- ./breakslights < $< > /dev/null

%.avr.o:	%.c
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.elf:
		$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

%.hex:		%.elf
		$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.size:		%
		$(SIZE) $<

%.flash:	%
		$(AVRDUDE) $(ADFLAGS) -e -U flash:w:$<:i

default:	$(TARGETS) test pylint

test:		$(addsuffix .reg,$(REGRESSION_TESTS)) \
			$(addsuffix .unit,$(UNIT_TESTS))

unit/hue:	CPPFLAGS += -I.
unit/hue:	unit/hue.o animation.o pixel.o

breakslights:	pixel.o ring.o animation.o comms.o machine.o breakslights.o

breakslights.elf:	pixel.avr.o ring.avr.o animation.avr.o comms.avr.o \
				neopixel.avr.o machine.avr.o \
				uart.avr.o common.avr.o breakslights.avr.o

flash:		breakslights.hex.flash

pylint:
	pylint -E breakslights.py render.py player.py

clean:
	rm -f *.o $(TARGETS) $(UNIT_TESTS)
