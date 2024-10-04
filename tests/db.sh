
#!/bin/sh
#
# db.sh - Test database operations
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


# --- Erased database ---------------------------------------------------------

empty erased "db open" "db stats" "db blocks" <<EOF
total 2048 invalid 0 data 0
erased 2039 deleted 0 empty 0
D8
EOF

# --- Create a new entry ------------------------------------------------------

empty new "db open" "db new blah" "db stats" "db blocks" <<EOF
9
total 2048 invalid 0 data 1
erased 2038 deleted 0 empty 0
D8 D9
EOF

# --- Create then delete an entry ---------------------------------------------

empty new-delete "db open" "db new blah" "db delete blah" "db stats" \
    "db blocks" <<EOF
9
total 2048 invalid 0 data 0
erased 2038 deleted 1 empty 0
D8 X9
EOF

# --- Create then change an entry ---------------------------------------------

empty new-change "db open" "db new blah" "db change blah" "db stats" \
    "db blocks" <<EOF
9
10
total 2048 invalid 0 data 1
erased 2037 deleted 1 empty 0
D8 X9 D10
EOF

# --- One existing entry ------------------------------------------------------

json <<EOF
[ { "id":"id", "user":"user" } ]
EOF

run existing "db open" "db stats" "db blocks" <<EOF
total 2048 invalid 0 data 1
erased 2038 deleted 0 empty 0
D8 D9
EOF

# --- Delete existing entry ---------------------------------------------------

json <<EOF
[ { "id":"id", "user":"user" } ]
EOF

run existing-delete "db open" "db delete id" "db stats" "db blocks" <<EOF
total 2048 invalid 0 data 0
erased 2038 deleted 1 empty 0
D8 X9
EOF

# --- Change field in existing entry ------------------------------------------

json <<EOF
[ { "id":"id", "user":"user" } ]
EOF

run existing-change "db open" "db change id" "db stats" "db blocks" <<EOF
10
total 2048 invalid 0 data 1
erased 2037 deleted 1 empty 0
D8 X9 D10
EOF

# --- Remove field from existing entry ----------------------------------------

json <<EOF
[ { "id":"id", "user":"user" } ]
EOF

run existing-remove "db open" "db remove id" "db stats" "db blocks" <<EOF
10
total 2048 invalid 0 data 1
erased 2037 deleted 1 empty 0
D8 X9 D10
EOF

# --- Replace obsolete entry, base --------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" } ]
EOF

run update-obsolete-base "db open" "db dump" <<EOF
a -
b -
EOF

# --- Replace obsolete entry, no change ---------------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":"a" } ]
EOF

run update-obsolete-same "db open" "db dump" <<EOF
a -
b -
EOF

# --- Replace obsolete entry, order changes -----------------------------------

json <<EOF
[ { "id":"a" }, { "id":"b" }, { "id":"a", "prev":"b" } ]
EOF

run update-obsolete-reorder "db open" "db dump" <<EOF
b -
a b
EOF

# --- Directory: one entry ----------------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] } ]
EOF

run dir-1 "db open" "db dump" <<EOF
a -
b -
	c -
EOF

# --- Directory: two entries --------------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] }, { "id":["b", "d"] } ]
EOF

run dir-2 "db open" "db dump" <<EOF
a -
b -
	c -
	d -
EOF

# --- Directory: two directories ----------------------------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] }, { "id":["c", "d"] } ]
EOF

run dir-1-1 "db open" "db dump" <<EOF
a -
b -
	c -
c -
	d -
EOF

# --- Directory: two directories, add to 1st ----------------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] }, { "id":["c", "d"] }, { "id":["b", "d"] } ]
EOF

run dir-2-1 "db open" "db dump" <<EOF
a -
b -
	c -
	d -
c -
	d -
EOF

# --- Directory: two directories, add to 1st, then 2nd ------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] }, { "id":["c", "d"] }, { "id":["b", "d"] },
  { "id":["c", "x"] } ]
EOF

run dir-2-2 "db open" "db dump" <<EOF
a -
b -
	c -
	d -
c -
	d -
	x -
EOF

# --- Directory: two directories, add to 2nd, then 1st ------------------------

json <<EOF
[ { "id":"a" }, { "id":["b", "c"] }, { "id":["c", "d"] }, { "id":["c", "a"] },
  { "id":["b", "x"] } ]
EOF

run dir-2-2b "db open" "db dump" <<EOF
a -
b -
	c -
	x -
c -
	a -
	d -
EOF
