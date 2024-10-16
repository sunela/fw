#!/bin/sh
#
# tsort.sh - Test topological sorting of the database
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

usage_run()
{
	echo "usage: $0 [-D] [-f] title [command ...]" 1>&2
	exit 1
}


run()
{
	local debug=
	local fail=false

	while [ "$1" ]; do
		case "$1" in
		-D)	debug=-D;;
		-f)	fail=true;;
		-*)	usage_run;;
		*)	break;;
		esac
		shift
	done
	
	local title=$1
	local s="../sim $debug -q -C 'db dummy'"

	shift
	echo -n "$title: " 1>&2

	if [ "$select" -a "$select" != "$title" ]; then
		echo skipped 1>&2
		return
	fi

	for n in "$@"; do
		s="$s 'db $n'"
	done
	if $gdb; then
		eval gdb --args $s "'db sort' 'db dump'" </dev/tty
		exit
	fi
	# @@@ should also fail if fail=true but the command had exit status 0.
	if ! eval $s "'db sort' 'db dump'" >_out 2>&1; then
		if $fail; then
			sed -i 's/^.*Assertion //;/^Aborted/d' _out
		else
			echo FAILED 1>&2
			exit 1
		fi
	fi
	if diff -u - _out >_diff; then
		echo PASSED 1>&2
		rm -f _diff
	else
		echo FAILED 1>&2
		cat _diff 1>&2
		exit 1
	fi

	[ "$select" ] && exit
}


usage()
{
	echo "usage: $0 [--gdb] [-x] [test-name]" 1>&2
	exit 1
}


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

# === abc(de), test setup =====================================================

#
# Sections with a === separator introduce a new SETUP sequence at the
# beginning, then may use it in several tests.
#

SETUP="'add a' 'add b a' 'mkdir c b' 'cd c' 'add d' 'add e d'"

eval run "'abc(de)'" "$SETUP" <<EOF
a -
b a
c b
	d -
	e d
EOF

# --- a:bc(dea) ---------------------------------------------------------------

eval run "'a:bc(dea)'" "$SETUP" cd \
    "'move-from a'" "'cd c'" move-before <<EOF
b -
c b
	d -
	e d
	a e
EOF

# --- a:bc(dae) ---------------------------------------------------------------

eval run "'a:bc(dae)'" "$SETUP" cd \
    "'move-from a'" "'cd c'" "'move-before e'" <<EOF
b -
c b
	d -
	a d
	e a
EOF

# --- a:bc(ade) ---------------------------------------------------------------

eval run "'a:bc(ade)'" "$SETUP" cd \
    "'move-from a'" "'cd c'" "'move-before d'" <<EOF
b -
c b
	a -
	d a
	e d
EOF

# --- d:abdc(e) ---------------------------------------------------------------

eval run "'d:abdc(e)'" "$SETUP" cd \
    "'cd c'" "'move-from d'" cd "'move-before c'" <<EOF
a -
b a
d b
c d
	e -
EOF

# --- d:abc(e)d ---------------------------------------------------------------

eval run "'d:abc(e)d'" "$SETUP" cd \
    "'cd c'" "'move-from d'" cd move-before <<EOF
a -
b a
c b
	e -
d c
EOF

# --- cannot move directory into itself ---------------------------------------

eval run -f "move-itself" "$SETUP" cd \
    "'move-from c'" "'cd c'" move-before <<EOF
\`e != db->dir' failed.
EOF

# === ac(d(e)), test setup ====================================================

SETUP="'add a' 'mkdir c a' 'cd c' 'mkdir d' 'cd d' 'add e'"
 
eval run "'ac(d(e))'" "$SETUP" <<EOF
a -
c a
	d -
		e -
EOF

# --- cannot move directory into child ----------------------------------------

eval run -f "move-to-child" "$SETUP" cd cd \
    "'move-from c'" "'cd c'" "'cd d'" pwd move-before <<EOF
\`!db_is_descendent(e, db->dir)' failed.
EOF

# -----------------------------------------------------------------------------

if [ "$select" ]; then
	echo "$select: not found" 1>&2
	exit 1
fi
