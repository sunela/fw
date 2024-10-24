#!/bin/bash
#
# ui-rmt.sh - "Remote control" operations
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# https://stackoverflow.com/a/28776166
(return 0 2>/dev/null) && sourced=true || sourced=false
$sourced || . ./Common "$@"

# --- Remote ------------------------------------------------------------------

accounts remote "long 200 72" "$ACCOUNTS_REMOTE"

# --- Remote (reveal) ---------------------------------------------------------

RDOP_REVEAL=05
FIELD_PASSWORD=05
NO="tap 69 256"
YES="tap 175 256"

accounts rmt-reveal "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD"

# --- Remote (reveal, no) -----------------------------------------------------

accounts rmt-reveal-no "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD" "$NO"

# --- Remote (reveal, yes) ----------------------------------------------------

accounts rmt-reveal-yes "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD" "$YES"

# --- Remote (set time, advance by 30 seconds) --------------------------------

RDOP_SET_TIME=07
T_150000=1725462000
T_150030=`expr $T_150000 + 30`
T_120000=`expr $T_150000 - 3 \* 3600`
T_010000=`expr $T_150000 + 10 \* 3600`

t_bytes()
{
	local tmp=$1
	local i=0

	while [ $i -lt 8 ]; do
		[ "$i" -gt 0 ] && printf " "
		printf "%02x" `expr $tmp % 256`
		tmp=`expr $tmp / 256`
		i=`expr $i + 1`
	done
}

accounts rmt-set-time-30s "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_150030`"

# --- Remote (set time, back 3h) ----------------------------------------------

accounts rmt-set-time-3h "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_120000`"

# --- Remote (set time, advance by 10h) ---------------------------------------

accounts rmt-set-time-10h "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_010000`"

# --- account (HOTP revealed)--------------------------------------------------

saved_mode=$mode
mode=run
if ! page_inner -n hotp-reveal-twice \
    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "drag 158 243 159 170" "tap 50 221" "tap 38 80"; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	accounts hotp-reveal-twice \
	    "drag 158 243 159 170" "tap 50 221" "tap 38 80"
fi

# =============================================================================

$sourced || atend
