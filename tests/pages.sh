#!/bin/sh
#
# pages.sh - Show/record/compare UI pages
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

#
# We generate PPM because its easy, and store as PNG because it's
# space-efficient.
#


#
# Image comparison:
# https://stackoverflow.com/q/5132749/11496135
#

#
# Make PNG reproducible:
# https://superuser.com/a/1220703
#
reproducible="-define png:exclude-chunks=date,time"


json()
{
	"$top/tools/accenc.py" /dev/stdin >"$dir/_db" ||
	    { rm -f "$dir/_dn"; exit 1; }
}


page_inner()
{
	local json=

	while [ "$1" ]; do
		case "$1" in
		-j)	json=$2
			shift 2;;
		*)	break;;
		esac
	done

	local mode=$1
	local name=$2
	shift 2

	if [ "$mode" = names ]; then
		echo $name
		return
	fi

	[ "$mode" = last ] || [ -z "$select" -o "$name" = "$select" ] || return

	echo === $name ===

	if [ ! -r "$dir/_db" ]; then
		if [ "$json" ]; then
			echo "$json" |
			    "$top/tools/accenc.py" /dev/stdin >"$dir/_db" ||
			    exit
		else
			"$top/tools/accenc.py" "$top/accounts.json" \
			    >"$dir/_db" || exit
		fi
	fi
	if [ "$mode" = interact ]; then
		$top/sim $quiet -d "$dir/_db" -C "$@" interact
		exit
	else
		$top/sim $quiet -d "$dir/_db" -C "$@" "screen $dir/_tmp.ppm" ||
		    exit
	fi

	case "$mode" in
	last)	return;;
	show)	display "$dir/_tmp.ppm";;
	run)	;;
	test)	convert $reproducible "$dir/_tmp.ppm" "$dir/_tmp.png"
		[ "`md5sum <\"$dir/$name.png\"`" = \
		  "`md5sum <\"$dir/_tmp.png\"`" ] || {
			compare "$dir/$name.png" "$dir/_tmp.png" - |
			    display
			exit
		}
		rm -f "$dir/_tmp.png";;
	store)	convert $reproducible "$dir/_tmp.ppm" "$dir/$name.png" || exit;;
	esac
}


page()
{
	page_inner "$@"
	local rc=$?
	rm -f "$dir/_tmp.ppm" "$dir/_db"
	return $rc
}


usage()
{
	cat <<EOF 1>&2
usage: $0 [-v] [-x] [run|show|interact|last|test|store|names [page-name]]

-v  enable debug output of the simulator
-x  set shell command tracing with set -x
EOF
	exit 1
}


self=`which "$0"`
dir=`dirname "$self"`
top=$dir/..

quiet=-q

while [ "$1" ]; do
	case "$1" in
	-v)	quiet=
		shift;;
	-x)	set -x
		shift;;
	-*)	usage;;
	*)	break;;
	esac
done

case "$1" in
run|show|test|store|last|names)
	mode=$1;;
inter|interact)
	mode=interact;;
"")	mode=test;;
*)	usage;;
esac

select=$2

rm -f "$dir/_db"

# --- on ----------------------------------------------------------------------

page $mode on "random 1" button

# --- pin ---------------------------------------------------------------------

PIN_1="tap 123 71"
PIN_2="tap 49 135"
PIN_3="tap 130 252"
PIN_4="tap 125 135"
PIN_NEXT="tap 191 252"

page $mode pin "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4"

# --- bad pin (once) ----------------------------------------------------------

page $mode pin-bad1 "random 1" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- bad pin (thrice) --------------------------------------------------------

page $mode pin-bad3 "random 3" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- four seconds into cooldown  ---------------------------------------------

page $mode cool-4s "random 3" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "tick 400"

# --- accounts ----------------------------------------------------------------

accounts()
{
	local opts=

	while [ "$1" ]; do
		case "$1" in
		-j)	opts="$opts -j $2"
			shift 2;;
		*)	break;;
		esac
	done

	local mode=$1
	local name=$2
	shift 2

	page $opts $mode $name \
	    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" \
	    "$PIN_NEXT" "$@"
}


accounts $mode accounts

# --- accounts scrolled up ----------------------------------------------------

accounts $mode accounts-up "drag 158 243 159 180"

# --- accounts (empty) --------------------------------------------------------

accounts -j "[]" $mode accounts-empty

# --- account (demo)-----------------------------------------------------------

accounts $mode account-demo "tap 86 67"

# --- account (HOTP hidden) ---------------------------------------------------

accounts $mode account-hotp "drag 158 243 159 196" "tap 50 221"

# --- account (HOTP revealed)--------------------------------------------------

accounts $mode account-hotp-reveal \
    "drag 158 243 159 196" "tap 50 221" "tap 38 80"

# --- account (TOTP)-----------------------------------------------------------

# Unix time 1716272769:
# UTC 2024-05-21 06:26:09
# Code 605617

accounts $mode account-totp \
    "time 1716272769" "drag 158 243 159 180" "tap 41 241" tick

# --- accounts overlay (top) --------------------------------------------------

accounts $mode accounts-over-top "long 201 23"

# --- accounts overlay (demo) --------------------------------------------------

accounts $mode accounts-over-demo "long 45 69"

# ---  account overlay (demo) --------------------------------------------------

accounts $mode account-demo-top-over "tap 86 67" "long 201 23"

# ---  accounts overlay add (demo) ---------------------------------------------

accounts $mode accounts-demo-add "long 45 69" "tap 153 110"

# ---  accounts overlay add M, level 1 (demo) ---------------------------------

accounts $mode accounts-demo-add-m1 "long 45 69" "tap 153 110" "tap 200 136"

# ---  accounts overlay add M, level 2 (demo) ---------------------------------

accounts $mode accounts-demo-add-m2 "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81"

# ---  accounts overlay add Me ------------------------------------------------

accounts $mode accounts-demo-add-me "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137"

# ---  accounts added Me ------------------------------------------------------

accounts $mode accounts-demo-added-me "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247"

# ---  account Me -------------------------------------------------------------

accounts $mode account-me "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247" \
    "tap 23 68"

# ---  account Me: fields list ------------------------------------------------

accounts $mode account-me-fields "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247" \
    "tap 23 68" "tap 119 165"

# ---  account Me: enter Password ---------------------------------------------

accounts $mode account-me-pw "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247" \
    "tap 23 68" "tap 119 165" "tap 71 167"

# ---  account Me: password Secr3t --------------------------------------------

ENTRY_1="tap 43 84"
ENTRY_2="tap 119 84"
ENTRY_3="tap 200 84"
ENTRY_4="tap 43 137"
ENTRY_5="tap 119 137"
ENTRY_6="tap 200 137"
ENTRY_7="tap 43 191"
ENTRY_8="tap 119 191"
ENTRY_9="tap 200 191"
ENTRY_L="tap 43 245"
ENTRY_0="tap 119 245"
ENTRY_R="tap 200 245"

accounts $mode account-me-pw-secret "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247" \
    "tap 23 68" "tap 119 165" "tap 71 167" \
    "$ENTRY_7" "$ENTRY_7" "$ENTRY_3" "$ENTRY_5" "$ENTRY_2" "$ENTRY_6" \
    "$ENTRY_7" "$ENTRY_6" "$ENTRY_3" "$ENTRY_0" "$ENTRY_8" "$ENTRY_4"

# ---  account Me: password added ---------------------------------------------

accounts $mode account-me-pw-added "long 45 69" "tap 153 110" \
    "tap 200 136" "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247" \
    "tap 23 68" "tap 119 165" "tap 71 167" \
    "$ENTRY_7" "$ENTRY_7" "$ENTRY_3" "$ENTRY_5" "$ENTRY_2" "$ENTRY_6" \
    "$ENTRY_7" "$ENTRY_6" "$ENTRY_3" "$ENTRY_0" "$ENTRY_8" "$ENTRY_4" \
    "$ENTRY_R"

# --- setup -------------------------------------------------------------------

accounts $mode setup "long 201 23" "tap 152 141"

# --- setup time --------------------------------------------------------------

accounts $mode setup-time "time 1716272769" \
    "long 201 23" "tap 152 141" "tap 93 119"

# --- setup storage -----------------------------------------------------------

accounts $mode setup-storage "long 201 23" "tap 152 141" "tap 81 164"

# --- delete account (swipe not started) --------------------------------------

accounts $mode account-demo-top-delete "tap 86 67" "long 201 23" \
    "tap 193 142"

# --- delete account (yellow) -------------------------------------------------

accounts $mode account-demo-top-delete-yellow "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 116 200"

# --- delete account (green) --------------------------------------------------

accounts $mode account-demo-top-delete-green "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 200"

# --- deleted account ---------------------------------------------------------

accounts $mode account-demo-deleted "tap 86 67" "long 201 23" \
    "tap 193 142" "drag 53 189 180 200"

# --- delete account (red) ---------------------------------------------------

accounts $mode account-demo-top-delete-red "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 150"

# --- account overlay (password field) ---------------------------------------

accounts $mode account-demo-pw-over "tap 86 67" "long 199 119"

# --- delete field (swipe not started) ---------------------------------------

accounts $mode account-demo-pw-delete "tap 86 67" "long 199 119" \
    "tap 153 174"

# --- deleted field  ----------------------------------------------------------

accounts $mode account-demo-pw-deleted "tap 86 67" "long 199 119" \
    "tap 153 174" "drag 53 189 180 200"

# --- account overlay (bottom) ------------------------------------------------

accounts $mode account-demo-bottom-over "tap 86 67" "long 107 194"

# --- account overlay (bottom) ------------------------------------------------

accounts $mode account-demo-bottom-over "tap 86 67" "long 107 194"

# --- account edit password (Geheimx) -----------------------------------------

# Adding an "x" to "Geheim" makes the text too long for centering, but doesn't
# yet require cutting any off-screen part.

accounts $mode account-demo-pw-geheimx "tap 86 67" "long 199 119" \
    "tap 88 172" "$ENTRY_9" "$ENTRY_5"

# --- account show secret (HOTP) ----------------------------------------------

# The secret is a long base32 string that gets cut off at the screen edge.

accounts $mode account-hotp-secret "drag 158 243 159 196" "tap 50 221" \
    "long 75 66" "tap 91 173"

# --- account with "comment" field --------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw",
    "comment":"comment" } ]
EOF

accounts $mode account-comment "tap 86 67"

# --- account with 2nd password field -----------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts $mode account-pw2 "tap 86 67"

# --- account with 2nd password and TOTP --------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts $mode account-pw2-totp "time 0" "tap 86 67"

# -----------------------------------------------------------------------------

[ "$1" = last ] && display "$dir/_tmp.ppm"

#
# Tentative naming convention:
#
# - page: page which, together with an optional qualifier, uniquely defines
#   where and in what context we are, e.g., account, setup
#   Here, overlays don't count as pages;
# - page-qualifier: if there are different pages of a type, e.g., account-HOTP
# - item-on-page: if we reference a specific item on the page, e.g.,
#   accounts-demo-...
# - sub-page: if we invoke a sub-page on a main page, but are still in the
#   process of reaching a new page, e.g., accounts-top-over
#   If we are at an overlay, use "over". If we have progressed past an overlay,
#   use the option selected (or consider this a new page), e.g., "add" would be
#   somewhere > Overlay > Add, because what happens may depend on "somewhere",
#   but we reduce ... > Setup to just "setup", because it's the same
#   everywhere.
#   Can also be used to indicate a point in a sequence, e.g.,
#   accounts-demo-added-me, where we are interested in what the accounts list
#   looks like after having added "Me".
# - context at this point, e.g., m2 for text entry with the two letters "Me"
#
# This is probably too complicated. Revisit the naming issue later.
#
