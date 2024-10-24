#!/bin/bash
#
# ui-pin.sh - PIN entry and change
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# https://stackoverflow.com/a/28776166
(return 0 2>/dev/null) && sourced=true || sourced=false
$sourced || . ./Common "$@"

# --- on ----------------------------------------------------------------------

page on "random 1" button

# --- pin ---------------------------------------------------------------------

page pin "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4"

# --- bad pin (once) ----------------------------------------------------------

page pin-bad1 "random 1" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- bad pin (thrice) --------------------------------------------------------

page pin-bad3 "random 3" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- four seconds into cooldown  ---------------------------------------------

add cool-4s "tick 400"

# === PIN change, old PIN, empty ==============================================

accounts change-old \
    "long 201 23" "tap 152 141" "tap 86 70"

# --- PIN change, old PIN, cancel ---------------------------------------------

save
add change-old-cancel "$ENTRY_L"
restore

# --- PIN change, old PIN, first digit ----------------------------------------

add change-old-1 "$ENTRY_1"

# --- PIN change, invalid PIN  ------------------------------------------------

save
add change-invalid "$ENTRY_1" "$ENTRY_1" "$ENTRY_1" "$ENTRY_R"
restore

# --- PIN change, new PIN  ----------------------------------------------------

add change-new "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# --- PIN change, new PIN, cancel  --------------------------------------------

save
add change-new-cancel "$ENTRY_L"
restore

# --- PIN change, new PIN, first digit  ---------------------------------------

add change-new-first "$ENTRY_1"

# --- PIN change, same PIN  ---------------------------------------------------

save
add change-same "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
restore

# --- PIN change, new PIN, all six digits  ------------------------------------

add change-new-all "$ENTRY_2" "$ENTRY_9" "$ENTRY_5" "$ENTRY_8" "$ENTRY_0"

# --- PIN change, confirm PIN -------------------------------------------------

add change-confirm "$ENTRY_R"

# --- PIN change, confirm PIN, cancel -----------------------------------------

save
add change-confirm-cancel "$ENTRY_L"
restore

# --- PIN change, match -------------------------------------------------------

save
add change-match \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_9" "$ENTRY_5" "$ENTRY_8" "$ENTRY_0" \
    "$ENTRY_R"
restore

# --- PIN change, mismatch ----------------------------------------------------

add change-mismatch \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_9" "$ENTRY_6" "$ENTRY_8" "$ENTRY_0" \
    "$ENTRY_R"

# =============================================================================

$sourced || atend
