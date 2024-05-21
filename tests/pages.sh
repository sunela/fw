#!/bin/sh -x
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


page()
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

	[ "$mode" = last ] || [ -z "$select" -o "$name" = "$select" ] || return

	echo === $name ===

	if [ "$json" ]; then
		echo "$json" |
		    "$top/tools/accenc.py" /dev/stdin >"$dir/_db" || exit
	else
		"$top/tools/accenc.py" "$top/accounts.json" >"$dir/_db" || exit
	fi
	$top/sim -d "$dir/_db" -C "$@" "screen $dir/_tmp.ppm" || exit

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

	rm -f "$dir/_tmp.ppm" "$dir/_db"
}


usage()
{
	echo "usage: $0 [run|show|last|test|store [page-name]]" 1>&2
	exit 1
}


self=`which "$0"`
dir=`dirname "$self"`
top=$dir/..

case "$1" in
run|show|test|store|last)
	mode=$1;;
"")	mode=test;;
*)	usage;;
esac

select=$2


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

# --- account (HOTP)-----------------------------------------------------------

# Unix time 1716272769:
# UTC 2024-05-21 06:26:09
# Code 605617

accounts $mode account-totp \
    "time 1716272769" "drag 158 243 159 180" "tap 41 241" tick
#"tap 50 221" "tap 38 80"

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
    "tap 200 136"  "tap 42 81"

# ---  accounts overlay add Me ------------------------------------------------

accounts $mode accounts-demo-add-me "long 45 69" "tap 153 110" \
    "tap 200 136"  "tap 42 81" "tap 194 83" "tap 119 137"

# ---  accounts overlay added Me ----------------------------------------------

accounts $mode accounts-demo-added-me "long 45 69" "tap 153 110" \
    "tap 200 136"  "tap 42 81" "tap 194 83" "tap 119 137" "tap 201 247"

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
