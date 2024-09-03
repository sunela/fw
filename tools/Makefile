#
# tools/Makefile - Build host tools for testing connectivity
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

CFLAGS = -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations
OBJS = io.o usbopen.o fake-rmt.o
LDLIBS = -lusb

include ../Makefile.c-common


.PHONY:	all clean spotless

all::	io

io:	$(OBJS)
	$(CC) $(LDLAGS) -o $@ $(OBJS) $(LDLIBS)

spotless::
	rm -f io
