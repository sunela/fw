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

	for n in "$@"; do
		s="$s '$n'"
	done
	if ! eval $s 2>&1 >_out; then
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
	echo "usage: $0 [-x]" 1>&2
	exit 1
}


self=`which "$0"`
dir=`dirname "$self"`
top=$dir/..

while [ "$1" ]; do
	case "$1" in
	-x)	set -x;;
	-*)	usage;;
	*)	break;;
	esac
	shift
done

[ "$1" ] && usage


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

# --- Root directory with subdirectory, go to sub then back -------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":["c","1"] }, { "id":["c","2"] } ]
EOF

run sub-back "db open" "db cd c" "db pwd" "db cd" "db pwd" <<EOF
c
(top)
EOF

