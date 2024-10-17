#!/bin/sh
#
# dir.sh - Test directory operations
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#


PK=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA====


run()
{
	local debug=

	if [ "$1" = -D ]; then
		debug=-D
		shift
	fi

	local title=$1
	local s="../sim $debug -q -d "$dir/_db" -C"

	shift
	echo -n "$title: " 1>&2

	if [ "$select" -a "$select" != "$title" ]; then
		echo skipped 1>&2
		return
	fi

	for n in "$@"; do
		s="$s '$n'"
	done
	if $gdb; then
		eval gdb --args $s </dev/tty
		exit
	fi

	if ! eval $s 2>&1 >_out; then
		echo FAILED 1>&2
		exit 1
	else
		if diff -u - _out >_diff; then
			echo PASSED 1>&2
			rm -f _diff
		else
			echo FAILED 1>&2
			cat _diff 1>&2
			exit 1
		fi
	fi

	[ "$select" ] && exit
}


json()
{
	"$top/tools/accenc.py" /dev/stdin $PK >"$dir/_db" || exit
}


empty()
{
	echo "[]" | json
	run "$@"
}


usage()
{
	echo "usage: $0 [--gdb] [-x] [test-name]" 1>&2
	exit 1
}


self=`which "$0"`
dir=`dirname "$self"`
top=$dir/..

gdb=false

while [ "$1" ]; do
	case "$1" in
	--gdb)	gdb=true;;
	-x)	set -x;;
	-*)	usage;;
	*)	break;;
	esac
	shift
done

select=$1
[ "$2" ] && usage


# --- Empty root directory ----------------------------------------------------

json <<EOF
[]
EOF

run empty "db open" "db ls" <<EOF
EOF

# --- Flat root directory -----------------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":"c" } ]
EOF

run root-flat "db open" "db ls" <<EOF
a
b
c
EOF

# --- Root directory, pwd -----------------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":"c" } ]
EOF

run root-pwd "db open" "db pwd" <<EOF
(top)
EOF

# --- Root directory with subdirectory, list root -----------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run root-sub "db open" "db ls" <<EOF
a
b
c
EOF

# --- Root directory with subdirectory, sort, list root -----------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run root-sub-sort "db open" "db sort" "db ls" <<EOF
a
b
c
EOF

# --- Root directory with subdirectory, pwd in sub ----------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run sub "db open" "db cd c" "db pwd" <<EOF
c
EOF

# --- Root directory with subdirectory, list sub ------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run sub "db open" "db cd c" "db ls" <<EOF
1
2
EOF

# --- Root directory with subdirectory, sort, list sub ------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run sub "db open" "db sort" "db cd c" "db ls" <<EOF
1
2
EOF

# --- Root directory with subdirectory, go to sub then back -------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run sub-back "db open" "db cd c" "db pwd" "db cd" "db pwd" <<EOF
c
(top)
EOF

# --- Add entry to root directory, no prev ------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run top-add "db open" "db add b" "db dump" <<EOF
a -
b -
c -
e -
	1 -
	3 -
EOF

# --- Add entry to root directory, no prev, sort ------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run top-add-sort "db open" "db add b" "db sort" "db dump" <<EOF
a -
b -
c -
e -
	1 -
	3 -
EOF

# --- Add entry to subdirectory, no prev --------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run sub-add "db open" "db cd e" "db add 2" "db dump" <<EOF
a -
c -
e -
	1 -
	2 -
	3 -
EOF

# --- Add entry to subdirectory, no prev, sort --------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run sub-add-sort "db open" "db cd e" "db add 2" "db sort" "db dump" <<EOF
a -
c -
e -
	1 -
	2 -
	3 -
EOF

# --- Add entry to root directory, with prev ----------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run top-add-prev "db open" "db add b c" "db dump" <<EOF
a -
b c
c -
e -
	1 -
	3 -
EOF

# --- Add entry to root directory, with prev, sort ----------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

#
# The order we obtain is not the most intuitive result, but it is valid.
#

run top-add-prev-sort "db open" "db add b c" "db sort" "db dump" <<EOF
a -
c -
e -
	1 -
	3 -
b c
EOF

# --- Add entry to subdirectory, with prev ------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run sub-add-prev "db open" "db cd e" "db add 2 1" "db dump" <<EOF
a -
c -
e -
	1 -
	2 1
	3 -
EOF

# --- Add entry to subdirectory, with prev (1), sort --------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run sub-add-prev-sort "db open" "db cd e" "db add 2 1" "db sort" "db dump" <<EOF
a -
c -
e -
	1 -
	2 1
	3 -
EOF

# --- Add entry to subdirectory, with prev (3), sort --------------------------

json <<EOF
[ { "id":"a" }, { "id":"c" }, { "id":["e","1"] }, { "id":["e","3"] } ]
EOF

run sub-add-prev-sort "db open" "db cd e" "db add 2 3" "db sort" "db dump" <<EOF
a -
c -
e -
	1 -
	3 -
	2 3
EOF

# -----------------------------------------------------------------------------

if [ "$select" ]; then
	echo "$select: not found" 1>&2
	exit 1
fi
