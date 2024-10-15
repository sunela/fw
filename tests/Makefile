#
# tests/Makefile - Run all tests
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

#
# @@@ Our test cases are scattered over several different frameworks, Should
# try to unify them, at least a little.
#

.PHONY:	all over overview dup duplicates


all:
	./pages.sh
	./tsort.sh
	./dir.sh
	./move.sh
	./rmt.sh
	./db.sh
	./bip39.sh

over overview:
	montage -label '%t' `./pages.sh names | sed 's/$$/.png/'` - | display

dup duplicates:
	./pages.sh names | sort | uniq -d
