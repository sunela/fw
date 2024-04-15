#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim fw

FONTS = mono18.font mono24.font mono34.font mono36.font mono58.font

.PHONY:	all sim fw clean spotless

all:	$(FONTS:%=font/%) $(TARGETS)

sim:
	$(MAKE) -f Makefile.sim

fw:
	$(MAKE) -f Makefile.fw

$(FONTS:%=font/%): font/Makefile font/cvtfont.py
	$(MAKE) -C font

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
