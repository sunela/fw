#!/bin/bash
#
# ui-new.sh - New device setup (with BIP39 entry)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# https://stackoverflow.com/a/28776166
(return 0 2>/dev/null) && sourced=true || sourced=false
$sourced || . ./Common "$@"

# --- New device --------------------------------------------------------------

page -e new-on "random 1" button

# --- New device, cancel ------------------------------------------------------

page -e new-cancel "random 1" button \
    "$ENTRY_L"

# === New device, PIN =========================================================

page -e new-pin "random 1" button \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4"

# --- New device, confirm -----------------------------------------------------

add -e new-confirm "$ENTRY_R"

# --- New device, confirmed ---------------------------------------------------

add -e new-confirmed "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# --- New device, enter -------------------------------------------------------

add -e new-enter "$ENTRY_5"

# --- New device, mismatch ----------------------------------------------------

page -e new-mismatch "random 1" button \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R" \
    "$ENTRY_0" "$ENTRY_0" "$ENTRY_0" "$ENTRY_0" "$ENTRY_R"

# --- New device, repeat ------------------------------------------------------

add -e new-repeat "$ENTRY_5"

# --- accounts (empty) --------------------------------------------------------

accounts -j "[]" accounts-empty

# --- anncounts (empty), short swipe has no effect ----------------------------

accounts -j "[]" accounts-empty-short-swipe "drag 100 100 90 90"

# --- setup master secret -----------------------------------------------------

accounts setup-master "long 201 23" "tap 152 141" "tap 81 168"

# --- setup master secret, show pubkey ----------------------------------------

save
add setup-master-pubkey "tap 128 69"
restore

# --- setup master secret, PIN ------------------------------------------------

save
add setup-master-pin "tap 129 119"
restore

# --- setup master secret, show -----------------------------------------------

save
add setup-master-show "master scramble" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
restore

# --- setup master secret, show, back -----------------------------------------

add setup-master-show-back "master scramble" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R" \
    "drag 200 100 80 100"

# === set master, PIN =========================================================

#
# Sections that have their title marked with === instead of --- contain
# incremental sequences (add ...). Be careful when making changes, as they may
# detail later tests !
#

set_master()
{
	local opts=

	while [ "$1" ]; do
		case "$1" in
		-j)	opts="$opts -j $2"
			shift 2;;
		*)	break;;
		esac
	done

	local name=$1
	shift

	page $opts $name \
	    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
	    "long 201 23" "tap 152 141" "tap 81 168" "tap 71 172" "$@"
}

set_master sm-pin

# --- set-master, 1st, "paddle" -----------------------------------------------

add sm-1 "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
add sm-1-p "$ENTRY_6"
add sm-1-pa "$ENTRY_1"
add sm-1-pad "$ENTRY_2"
add sm-1-padd "$ENTRY_2"

# --- set-master, 2nd, "pig" --------------------------------------------------

FIRST="tap 70 139"

add sm-2 "$FIRST"
add sm-2-p "$ENTRY_6"
add sm-2-pi "$ENTRY_4"
add sm-2-pig "$ENTRY_4"
add sm-2-pig-accept "$ENTRY_R"

# --- set-master, 3nd, "ship" -------------------------------------------------

add sm-3 "$FIRST"
add sm-3-s "$ENTRY_8"
add sm-3-sh "$ENTRY_4"
add sm-3-shi "$ENTRY_4"
add sm-3-ship "$ENTRY_6"

# --- set-master, 4th, "cat"  -------------------------------------------------

# "FIRST" here is actually the second entry, since the list moves up

add sm-4 "$FIRST"
add sm-4-cat "$ENTRY_2" "$ENTRY_1" "$ENTRY_9"
add sm-4-cat-accept "$ENTRY_R"

# --- set-master, 5th, "sword" ------------------------------------------------

# We already know after the 3rd input that the word is "sword"

add sm-5 "$FIRST"
add sm-5-s "$ENTRY_8"
add sm-5-sw "$ENTRY_0"
add sm-5-swo "$ENTRY_6"
#add sm-5-swo-accept "$FIRST"

# --- set-master, 6th to 24th -------------------------------------------------

SECOND="tap 28 166"

add sm-6-duck "$FIRST" "$ENTRY_2" "$ENTRY_9" "$ENTRY_2"
add sm-7-way "$FIRST" "$ENTRY_0" "$ENTRY_1" "$ENTRY_0" "$ENTRY_R"
add sm-8-wave "$FIRST" "$ENTRY_0" "$ENTRY_1" "$ENTRY_0" "$ENTRY_3"
add sm-9-abandon "$FIRST" "$ENTRY_1" "$ENTRY_1" "$ENTRY_1" "$ENTRY_6"
add sm-10-cluster "$FIRST" "$ENTRY_2" "$ENTRY_5" "$ENTRY_9" "$ENTRY_8"
add sm-11-effort "$FIRST" "$ENTRY_3" "$ENTRY_3" "$ENTRY_3" "$ENTRY_6"
add sm-12-giggle "$FIRST" "$ENTRY_4" "$ENTRY_4" "$ENTRY_4" "$ENTRY_4"
add sm-13-mammal "$FIRST" "$ENTRY_5" "$ENTRY_1" "$ENTRY_5" "$ENTRY_5"
add sm-14-oppose "$FIRST" "$ENTRY_6" "$ENTRY_6" "$ENTRY_6" "$ENTRY_6"
add sm-15-rural "$FIRST" "$ENTRY_7" "$ENTRY_9" "$ENTRY_7"

# Note: with "squeeze", the "U" could be auto-completed, but the user would
# then have to pay attention to any such auto-completion happening, which may
# not always be easy. So it's better to keep the system more predictable.
add sm-16-squeeze "$FIRST" "$ENTRY_8" "$ENTRY_7" "$ENTRY_9" "$ENTRY_3"

add sm-17-turtle "$FIRST" "$ENTRY_9" "$ENTRY_9" "$ENTRY_7" "$ENTRY_9"
add sm-18-wrong "$FIRST" "$ENTRY_0" "$ENTRY_7" "$ENTRY_6"
add sm-19-orient "$FIRST" "$ENTRY_6" "$ENTRY_7" "$ENTRY_4" "$ENTRY_3"
add sm-20-toe "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_R"
add sm-21-unfair "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_1"
add sm-22-unfold "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_6"
add sm-23-merit "$FIRST" "$ENTRY_5" "$ENTRY_3" "$ENTRY_7" "$ENTRY_4"
add sm-24-almost "$SECOND" "$ENTRY_1" "$ENTRY_5" "$ENTRY_5" "$ENTRY_6"

save
add sm-24-fail "$FIRST"
restore

add sm-24-done "$SECOND"

# bip39 decode paddle pig ship cat sword duck way wave abandon cluster effort giggle mammal oppose rural squeeze turtle wrong orient toe unfair unfold merit almost
# 9ed4931811edc487be07bf00058d1ab0f86b372f669ceaffd67271ced1da62e0

# --- set-master, verify ------------------------------------------------------

# ENTRY_5 is just a random tap

add sm-verify "$ENTRY_5" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# =============================================================================

$sourced || atend
