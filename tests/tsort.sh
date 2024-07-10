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
	local s="../sim -q -C 'db dummy'"

	shift
	echo -n "$title: " 1>&2

	for n in "$@"; do
		s="$s 'db $n'"
	done
	if ! eval $s "'db sort' 'db dump'" 2>&1 >_out; then
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

run "alphabetic" "add a" "add b" "add c" <<EOF
a -
b -
c -
EOF

# --- Reversed alphabetic sequence --------------------------------------------

run "reversed alphabetic" "add c" "add b" "add a" <<EOF
a -
b -
c -
EOF

# --- Total order (forward) ---------------------------------------------------

run "total forward" "add a" "add b a" "add c b" <<EOF
a -
b a
c b
EOF

# --- Total order (forward, due to alphabetical insertion) --------------------

run "total forward alpha" "add c b" "add b a" "add a" <<EOF
a -
b a
c b
EOF

# --- Total order (backward) --------------------------------------------------

run "total backward" "add a b" "add b c" "add c" <<EOF
c -
b c
a b
EOF

# --- Partial order (tie) -----------------------------------------------------

run "partial tie" "add a" "add b a" "add c a" <<EOF
a -
b a
c a
EOF

# --- Partial order (incomplete) ----------------------------------------------

run "partial incomplete" "add a" "add b" "add c a" <<EOF
a -
b -
c a
EOF

# --- Partial reorder (incomplete) --------------------------------------------

run "partial reorder incomplete" "add a" "add b c" "add c" <<EOF
a -
c -
b c
EOF

# --- Loop (abc)---------------------------------------------------------------

run "loop (abc)" "add a c" "add b a" "add c b" <<EOF
a c
b a
c b
EOF

# --- Loop (cba)---------------------------------------------------------------

run "loop (cba)" "add a b" "add b c" "add c a" <<EOF
a b
c a
b c
EOF

# --- Disjoint (ab cd)---------------------------------------------------------

run "disjoint (ab cd)" "add a" "add b a" "add c" "add d c" <<EOF
a -
b a
c -
d c
EOF

# --- Disjoint (ab cd)---------------------------------------------------------

run "disjoint (ab dc)" "add a" "add b a" "add c d" "add d" <<EOF
a -
b a
d -
c d
EOF

# --- Empty database ----------------------------------------------------------

run "empty" <<EOF
EOF
