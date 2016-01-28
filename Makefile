CFLAGS ?= -Wall
CFLAGS += -D POSIX

ifdef DEBUG
	CFLAGS += -D DEBUG=1
	CFLAGS += -g
endif

.PHONY:		all test clean

default:	breakslights

breakslights:	ring.o animation.o comms.o machine.o breakslights.o

clean:
	rm -f *.o breakslights
