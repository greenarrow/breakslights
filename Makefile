CFLAGS ?= -Wall
CFLAGS += -D POSIX

ifdef DEBUG
	CFLAGS += -D DEBUG=1
	CFLAGS += -g
endif

VALGRIND = valgrind -q --error-exitcode=1 --leak-check=yes \
	--undef-value-errors=yes --show-reachable=yes --log-file=/dev/null

REGRESSION_TESTS = tests/fills.lc tests/strobe.lc tests/chase.lc

.PHONY:		all test clean

%.reg:		%
	./breakslights < $< | diff -u -- $<.stdout -
	$(VALGRIND) -- ./breakslights < $< > /dev/null

default:	breakslights test

test:		$(addsuffix .reg,$(REGRESSION_TESTS))

breakslights:	ring.o animation.o comms.o machine.o breakslights.o

clean:
	rm -f *.o breakslights
