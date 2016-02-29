CFLAGS ?= -Wall
CFLAGS += -D POSIX

ifdef DEBUG
	CFLAGS += -D DEBUG=1
	CFLAGS += -g
endif

VALGRIND = valgrind -q --error-exitcode=1 --leak-check=yes \
	--undef-value-errors=yes --show-reachable=yes --log-file=/dev/null

REGRESSION_TESTS = tests/fills.lc tests/strobe.lc tests/chase.lc \
	tests/mirror.lc tests/chopflash.lc \
	tests/ripple.lc tests/shadows.lc tests/multiprop.lc

.PHONY:		all test clean pylint

%.reg:		% breakslights
	./breakslights < $< | diff -u -- $<.stdout -
	$(VALGRIND) -- ./breakslights < $< > /dev/null

default:	breakslights test pylint

test:		$(addsuffix .reg,$(REGRESSION_TESTS))

breakslights:	pixel.o ring.o animation.o comms.o machine.o breakslights.o

pylint:
	pylint -E breakslights.py render.py

clean:
	rm -f *.o breakslights
