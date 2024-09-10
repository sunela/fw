
#!/bin/sh
#
# rmt.sh - Test the remote protocol
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#


PIN_1="tap 123 71"
PIN_2="tap 49 135"
PIN_3="tap 130 252"
PIN_4="tap 125 135"
PIN_NEXT="tap 191 252"

RDOP_UNKNOWN=ff
RDOP_LS=02
RDOP_SHOW=04
RDOP_REVEAL=05

FIELD_END=00
FIELD_USER=03
FIELD_EMAIL=04
FIELD_PW=05
FIELD_HOTP_SECRET=06
FIELD_HOTP_COUNTER=07
FIELD_TOTP_SECRET=08
FIELD_COMMENT=09
FIELD_PW2=0a

PK=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA====


run()
{
	local debug=

	if [ "$1" = -D ]; then
		debug=-D
		shift
	fi

	local title=$1
	local s="../sim $debug -q -d "$dir/_db" -C 'random 1' button"
	s="$s '$PIN_1' '$PIN_2' '$PIN_3' '$PIN_4' '$PIN_NEXT' 'rmt open'"

	shift
	echo -n "$title: " 1>&2

	"$top/tools/accenc.py" "$top/accounts.json" $PK >"$dir/_db" || exit

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


# --- Remote ls ---------------------------------------------------------------

run ls "rmt $RDOP_LS" <<EOF
demo
more
2nd
dummy1
dummy2
HOTP
TOTP
EOF

# --- Show demo ---------------------------------------------------------------

run show-demo "rmt $RDOP_SHOW demo" <<EOF
$FIELD_USER user@mail.com
$FIELD_PW
EOF

# --- Show unknown entry ------------------------------------------------------

run show-unknown "rmt $RDOP_SHOW demox" <<EOF
Error: Not found
EOF

# --- Reveal known entry and field --------------------------------------------

gdb="gdb --args </dev/tty" \
run reveal-demo-pw "rmt $RDOP_REVEAL demo $FIELD_PW" <<EOF
EOF

# --- Reveal unknown entry, known field ---------------------------------------

run reveal-unknown-pw "rmt $RDOP_REVEAL demox $FIELD_PW" <<EOF
Error: Not found
EOF

# --- Reveal known entry, already visible field -------------------------------

run reveal-demo-user "rmt $RDOP_REVEAL demo $FIELD_END" <<EOF
Error: Not found
EOF

# --- Reveal unknown entry, already visible field -----------------------------

run reveal-unknown-user "rmt $RDOP_REVEAL demox $FIELD_END" <<EOF
Error: Not found
EOF

# --- Unknown operation -------------------------------------------------------

run unknown "rmt $RDOP_UNKNOWN" <<EOF
Error: Bad request
EOF
