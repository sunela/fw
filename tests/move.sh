#!/bin/sh
#
# tsort.sh - Test topological sorting of the database
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

run()
{
	local debug=

	if [ "$1" = -D ]; then
		debug=-D
		shift
	fi

	local title=$1
	local s="../sim $debug -q -C 'db dummy'"

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


# --- c:acb (partial, doesn't work) -------------------------------------------

run "c:acb (partial)" "add a" "add b" "add c" "move c b" <<EOF
a -
b -
c a
EOF

# --- c:acb (total) -----------------------------------------------------------

run "c:acb (total)" "add a" "add b a" "add c b" "move c b" <<EOF
a -
c a
b c
EOF

# --- c:cba (total) -----------------------------------------------------------

run "c:cba (total)" "add a" "add b a" "add c b" "move c a" <<EOF
c -
a c
b a
EOF

# --- a:bac(total) ------------------------------------------------------------

run "a:bac (total)" "add a" "add b a" "add c b" "move a c" <<EOF
b -
a b
c a
EOF

# --- a:bca(total) -----------------------------------------------------------

run "a:bca (total)" "add a" "add b a" "add c b" "move a" <<EOF
b -
c b
a c
EOF

# --- b:bac(total) ------------------------------------------------------------

run "b:bca (total)" "add a" "add b a" "add c b" "move b a" <<EOF
b -
a b
c a
EOF

# --- b:acb(total) ------------------------------------------------------------

run "b:acb (total)" "add a" "add b a" "add c b" "move b" <<EOF
a -
c a
b c
EOF

# --- c:abc (move to itself)  -------------------------------------------------

run "c:abc (total)" "add a" "add b a" "add c b" "move c c" <<EOF
a -
b a
c b
EOF

# --- b:abc (move to itself)  -------------------------------------------------

run "b:abc (total)" "add a" "add b a" "add c b" "move b b" <<EOF
a -
b a
c b
EOF

# --- a:abc (move to itself)  -------------------------------------------------

run "a:abc (total)" "add a" "add b a" "add c b" "move a a" <<EOF
a -
b a
c b
EOF

# --- c:abc (move to end)  ----------------------------------------------------

run "c:abc (total, end)" "add a" "add b a" "add c b" "move c" <<EOF
a -
b a
c b
EOF

