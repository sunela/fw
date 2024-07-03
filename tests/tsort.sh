#!/bin/sh
#
# tsort.sh - Test topological sorting of the database
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

run()
{
	local title=$1
	local s="../sim -q -C dummy"

	shift
	echo -n "$title: " 1>&2

	for n in "$@"; do
		s="$s 'dummy $n'"
	done
	if ! eval $s "'dummy sort' 'dummy dump'" 2>&1 >_out; then
		echo "FAILED" 1>&2
		exit 1
	else
		if diff -u - _out >_diff; then
			echo "PASSED" 1>&2
			rm -f _diff
		else
			echo "FAILED" 1>&2
			cat _diff 1>&2
			exit 1
		fi
	fi
}


usage()
{
	echo "usage: $0 [-x]" 1>&2
	exit 1
}


while [ "$1" ]; do
	case "$1" in
	-x)	set -x;;
	-*)	usage;;
	*)	break;;
	esac
	shift
done

[ "$1" ] && usage


# --- Alphabetic sequence -----------------------------------------------------

run "alphabetic" a b c <<EOF
a -
b -
c -
EOF

# --- Reversed alphabetic sequence --------------------------------------------

run "reversed alphabetic" c b a <<EOF
a -
b -
c -
EOF

# --- Total order (forward) ---------------------------------------------------

run "total forward" a "b a" "c b" <<EOF
a -
b a
c b
EOF

# --- Total order (forward, due to alphabetical insertion) --------------------

run "total forward alpha" "c b" "b a" a <<EOF
a -
b a
c b
EOF

# --- Total order (backward) --------------------------------------------------

run "total backward" "a b" "b c" c <<EOF
c -
b c
a b
EOF

# --- Partial order (tie) -----------------------------------------------------

run "partial tie" a "b a" "c a" <<EOF
a -
b a
c a
EOF

# --- Partial order (incomplete) ----------------------------------------------

run "partial incomplete" a b "c a" <<EOF
a -
b -
c a
EOF

# --- Partial reorder (incomplete) --------------------------------------------

run "partial reorder incomplete" a "b c" c <<EOF
a -
c -
b c
EOF

# --- Loop (abc)---------------------------------------------------------------

run "loop (abc)" "a c" "b a" "c b" <<EOF
a c
b a
c b
EOF

# --- Loop (cba)---------------------------------------------------------------

run "loop (cba)" "a b" "b c" "c a" <<EOF
a b
c a
b c
EOF

# --- Disjoint (ab cd)---------------------------------------------------------

run "disjoint (ab cd)" a "b a" c "d c" <<EOF
a -
b a
c -
d c
EOF

# --- Disjoint (ab cd)---------------------------------------------------------

run "disjoint (ab dc)" a "b a" "c d" d <<EOF
a -
b a
d -
c d
EOF

# --- Empty database ----------------------------------------------------------

run "empty" <<EOF
EOF
