#!/bin/sh
#
# setenv.sh - Set the default build enviroment for board
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# BOARD: M1S_DOCK
[ -z "${BOARD}" ] && BOARD=M1S_DOCK

if [ "${BOARD}" == "M1S_DOCK" ]; then
	ln -sf Makefile.bl808 Makefile.sdk
	rm -f sdk
	ln -s boards/m1sdock sdk
fi
