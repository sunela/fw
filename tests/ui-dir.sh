#!/bin/bash
#
# ui-dir.sh - Directory operations
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# https://stackoverflow.com/a/28776166
(return 0 2>/dev/null) && sourced=true || sourced=false
$sourced || . ./Common "$@"

# === directories, scroll up ==================================================

accounts dir-up "drag 158 279 159 0"

# --- directories, go down the hierarchy ... ----------------------------------

zebra()
{
	local opts=

	while [ "$1" ]; do
		case "$1" in
		-k)	opts="$opts -k"
			shift;;
		*)	break;;
		esac
	done

	local name=$1
	shift

	accounts $opts $name "drag 158 279 159 0" "tap 93 204" "$@"
}


zebra dir-zebra
add dir-quagga "tap 70 171"

# --- ... and back up again ---------------------------------------------------

add dir-quagga-back "drag 200 100 50 100"
add dir-quagga-back2 "drag 200 100 50 120"
add dir-quagga-back3 "drag 200 100 50 80"

# === directories, delete accounts ============================================

TOP_OVER='"long 111 17"'
DELETE_ACC='"long 111 17" "tap 181 140" "drag 52 194 194 204"'
DELETE_DIR='"long 111 17" "tap 151 170" "drag 52 194 194 204"'

eval zebra dir-zebra-not-empty $TOP_OVER

eval zebra dir-del-selousi '"tap 70 171"' '"tap 114 118"' $DELETE_ACC
eval add dir-del-bohemi '"tap 115 70"' $DELETE_ACC

# --- directories, delete 2nd level subdirectory ------------------------------

eval add dir-del-quagga $DELETE_DIR

# --- directories, delete more accounts ---------------------------------------

eval add dir-del-capensis '"tap 115 70"' $DELETE_ACC

save
eval add dir-zebra-still-not-empty $TOP_OVER
restore

eval add dir-del-grevyi '"tap 115 70"' $DELETE_ACC

# --- directories, delete 1st level subdirectory ------------------------------

eval add dir-del-zebra $DELETE_DIR '"drag 158 279 159 0"'

# === directories, rename =====================================================

eval zebra dir-zebra-zeb $TOP_OVER '"tap 177 140"' \
    '"$ENTRY_L"' '"$ENTRY_L"' '"$ENTRY_R"'

saved_mode=$mode
mode=run
if ! add -n -k dir-zeb; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	eval zebra dir-zeb
fi

# --- Rename entry in directory -----------------------------------------------

eval zebra dir-grevyi-grey '"tap 79 120"' $TOP_OVER '"tap 120 138"' \
    '"$ENTRY_L"' '"$ENTRY_L"' '"$ENTRY_L"' '"$ENTRY_9"' '"$ENTRY_6"' \
    '"$ENTRY_R"'

saved_mode=$mode
mode=run
if ! add -n -k dir-grey; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	eval zebra dir-grey
fi

# === directories, add entry in directory =====================================

zebra zebra-add "long 125 126" "tap 52 162" \
    "$ENTRY_9" "$ENTRY_2" "$ENTRY_R"

saved_mode=$mode
mode=run
if ! add -n -k zebra-add-load; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	zebra zebra-add-load
fi

# === directories, long name ==================================================

json <<EOF
[ { "id":"AbcdeFghijKlmnoP", "dir":"" } ]
EOF

accounts -k dir-long "tap 119 69"
add dir-long-scroll "drag 200 10 100 10"

# === new directory, empty entry (X) ==========================================

accounts dir-empty-entry "long 100 100"  "tap 58 174" \
    "$ENTRY_9" "$ENTRY_2" "$ENTRY_R"

# --- new directory, empty directory entry ------------------------------------

add dir-empty-dir "tap 72 69" "tap 147 162"

# --- new directory, empty name -----------------------------------------------

save
add dir-empty-name "long 100 10" "tap 88 170" "$ENTRY_L"
restore

# --- new directory, empty, fields overlay ------------------------------------

add dir-empty-over "long 100 200"

# === Common definitions for subdirectory operations ==========================

LIST_1="80 82"
LIST_2="80 124"
LIST_3="80 169"
LIST_4="80 200"
BACK="drag 172 164 55 164"
MOVE="tap 129 168"	# from/to


#
# Directory tree structure (dirs.json):
#
# a
# b
# c/
#   d
#   e/
#     f
#     a
#

# === moving with subdirectories, accounts screens ============================

accounts -j dirs.json sub-top
add -j dirs.json sub-lvl1 "tap $LIST_3"
add -j dirs.json sub-lvl2 "tap $LIST_2"

# --- move c/e/f to c/ --------------------------------------------------------

add -j dirs.json sub-f-c "long $LIST_1" "$MOVE" "$BACK"

save
add -k -j dirs.json sub-f-c-1st "long $LIST_1" "$MOVE"
reload -j dirs.json sub-f-c-1st "tap $LIST_3"
restore

save
add -j dirs.json sub-f-c-2nd "long $LIST_2" "$MOVE"
reload -j dirs.json sub-f-c-2nd "tap $LIST_3"
restore

save
add -j dirs.json sub-f-c-3rd "long $LIST_3" "$MOVE"
reload -j dirs.json sub-f-c-3rd "tap $LIST_3"
restore

# --- move c/e/f to top-level -------------------------------------------------

add -j dirs.json sub-f-top "$BACK"

save
add -j dirs.json sub-f-top-1st "long $LIST_1" "$MOVE"
reload -j dirs.json sub-f-top-1st
restore

save
add -j dirs.json sub-f-top-2nd "long $LIST_2" "$MOVE"
reload -j dirs.json sub-f-top-2nd
restore

save
add -j dirs.json sub-f-top-3rd "long $LIST_3" "$MOVE"
reload -j dirs.json sub-f-top-3rd
restore

save
add -j dirs.json sub-f-top-4th "long $LIST_4" "$MOVE"
reload -j dirs.json sub-f-top-4th
restore

# === move c/d to c/e/ ========================================================

accounts -j dirs.json sub-d-e-from "tap $LIST_3" "long $LIST_1" "$MOVE"
add -j dirs.json sub-d-e "tap $LIST_2"

save
add -j dirs.json sub-d-e-1st "long $LIST_1" "$MOVE"
# Note: the selection in c/ changes from 2nd to 1st, since we moved the first
# entry.
reload -j dirs.json sub-d-e-1st "tap $LIST_3" "tap $LIST_1"
restore

save
add -j dirs.json sub-d-e-2nd "long $LIST_2" "$MOVE"
reload -j dirs.json sub-d-e-2nd "tap $LIST_3" "tap $LIST_1"
restore

save
add -j dirs.json sub-d-e-3rd "long $LIST_3" "$MOVE"
reload -j dirs.json sub-d-e-3rd "tap $LIST_3" "tap $LIST_1"
restore

# === move b to c/e/ ==========================================================

accounts -j dirs.json sub-b-e-from "long $LIST_2" "$MOVE"
add -j dirs.json sub-b-e "tap $LIST_3" "tap $LIST_2"

save
add -j dirs.json sub-b-e-1st "long $LIST_1" "$MOVE"
reload -j dirs.json sub-b-e-1st "tap $LIST_2" "tap $LIST_2"
restore

save
add -j dirs.json sub-b-e-2nd "long $LIST_2" "$MOVE"
reload -j dirs.json sub-b-e-2nd "tap $LIST_2" "tap $LIST_2"
restore

save
add -j dirs.json sub-b-e-3rd "long $LIST_3" "$MOVE"
reload -j dirs.json sub-b-e-3rd "tap $LIST_2" "tap $LIST_2"
restore

# === try to move a to c/e/ ===================================================

# Since there is already an entry "a" in c/e/, this would produce a duplicate
# name.

accounts -j dirs.json sub-a-e-from "long $LIST_1" "$MOVE"
add -j dirs.json sub-a-e "tap $LIST_3" "tap $LIST_2"
add -j dirs.json sub-a-e-to "long $LIST_1"
add -j dirs.json sub-a-e-try "$MOVE"

# --- try to move c/e/a to the top-level directory ----------------------------

# Again, the two "a" would clash.

accounts -j dirs.json sub-a-top-from "tap $LIST_3" "tap $LIST_2" \
    "long $LIST_2" "$MOVE"
add -j dirs.json sub-a-top "$BACK" "$BACK"
add -j dirs.json sub-a-top-to "long $LIST_1"
add -j dirs.json sub-a-top-try "$MOVE"

# === try to move c to c/ or c/e/ =============================================

accounts -j dirs.json sub-c-from "long $LIST_3" "$MOVE"
add -j dirs.json sub-c-c "tap $LIST_3"
add -j dirs.json sub-c-c-to "long $LIST_2"
add -j dirs.json sub-c-c-try "$MOVE"

add -j dirs.json sub-c-e "tap $LIST_2"
add -j dirs.json sub-c-e-to "long $LIST_1"
add -j dirs.json sub-c-e-try "$MOVE"

# === try to move c/e to c/e/ =================================================

accounts -j dirs.json sub-e-from "tap $LIST_3" "long $LIST_2" "$MOVE"
add -j dirs.json sub-e-e "tap $LIST_2"
add -j dirs.json sub-e-e-to "long $LIST_1"
add -j dirs.json sub-e-e-try "$MOVE"

# === move directory c/e to the top-level directory ===========================

accounts -j dirs.json sub-e-top-from "tap $LIST_3" "long $LIST_2" "$MOVE"
add -j dirs.json sub-e-top "$BACK"

save
add -j dirs.json sub-e-top-1st "long $LIST_1" "$MOVE"
add -j dirs.json sub-e-top-1st-in "tap $LIST_1"
reload -j dirs.json sub-e-top-1st-in "tap $LIST_1"
restore

save
add -j dirs.json sub-e-top-2nd "long $LIST_2" "$MOVE"
add -j dirs.json sub-e-top-2nd-in "tap $LIST_2"
reload -j dirs.json sub-e-top-2nd-in "tap $LIST_2"
restore

# =============================================================================

$sourced || atend
