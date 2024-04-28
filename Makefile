#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim fw sdk

FONTS = mono18.font mono24.font mono34.font mono36.font mono58.font

.PHONY:	all sim fw sdk flash clean spotless

all:	$(FONTS:%=font/%) $(TARGETS)

sim:
	$(MAKE) -f Makefile.sim

fw:
	$(MAKE) -f Makefile.fw

sdk:
	$(MAKE) -f Makefile.sdk
	$(MAKE) -C sdk

flash:
	$(MAKE) -C sdk flash

$(FONTS:%=font/%): font/Makefile font/cvtfont.py
	$(MAKE) -C font

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done
	$(MAKE) -C sdk clean

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
