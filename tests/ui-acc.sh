#!/bin/bash
#
# ui-acc.sh - Accounts
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

# https://stackoverflow.com/a/28776166
(return 0 2>/dev/null) && sourced=true || sourced=false
$sourced || . ./Common "$@"

# --- accounts ----------------------------------------------------------------

accounts accounts

# --- accounts scrolled up ----------------------------------------------------

accounts accounts-up "drag 158 243 159 117"

# --- account (demo)-----------------------------------------------------------

accounts account-demo "tap 86 67"

# --- account (HOTP hidden) ---------------------------------------------------

accounts account-hotp "drag 158 243 159 170" "tap 50 221"

# --- account (HOTP revealed)--------------------------------------------------

accounts account-hotp-reveal \
    "drag 158 243 159 170" "tap 50 221" "tap 38 80"

# --- account (TOTP)-----------------------------------------------------------

# Unix time 1716272769:
# UTC 2024-05-21 06:26:09
# Code 605617

accounts account-totp \
    "time 1716272769" "drag 158 243 159 130" "tap 41 241" tick

# --- accounts overlay (top) --------------------------------------------------

accounts accounts-over-top "long 201 23"

# --- accounts overlay (demo) --------------------------------------------------

accounts accounts-over-demo "long 45 69"

# ---  account overlay (demo) --------------------------------------------------

accounts account-demo-top-over "tap 86 67" "long 201 23"

# === accounts overlay add (demo) =============================================

ACCOUNTS_ADD="tap 60 169"
ACCOUNTS_MOVE="tap 120 168"
ACCOUNTS_CANCEL_MOVE="tap 193 177"
ACCOUNTS_REMOTE="tap 181 109"

accounts accounts-demo-add "long 45 69" "$ACCOUNTS_ADD"

# ---  accounts overlay add M, level 1 (demo) ---------------------------------

add accounts-demo-add-m1 "tap 200 136"

# ---  accounts overlay add M, level 2 (demo) ---------------------------------

add accounts-demo-add-m2 "tap 42 81"

# ---  accounts overlay add Me ------------------------------------------------

add accounts-demo-add-me "tap 194 83" "tap 119 137"

# ---  accounts added Me ------------------------------------------------------

add accounts-demo-added-me "tap 201 247"

# ---  account Me -------------------------------------------------------------

add account-me "tap 23 68"

# ---  account Me, verify that short swipe has no effect ----------------------

save
add account-me-short-swipe "drag 100 100 90 90"
restore

# ---  account Me, turn into directory ----------------------------------------

save
add mkdir-me "tap 145 158"
restore

save
saved_mode=$mode
mode=run
if ! add -n -k mkdir-me-load "tap 145 158"; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	accounts mkdir-me-load
fi
restore

# ---  account Me: fields list ------------------------------------------------

add account-me-fields "tap 119 165"

# ---  account Me: enter Password ---------------------------------------------

add account-me-pw "tap 71 167"

# ---  account Me: password Secr3t --------------------------------------------

add account-me-pw-secret \
    "$ENTRY_7" "$ENTRY_7" "$ENTRY_3" "$ENTRY_5" "$ENTRY_2" "$ENTRY_6" \
    "$ENTRY_7" "$ENTRY_6" "$ENTRY_3" "$ENTRY_0" "$ENTRY_8" "$ENTRY_4"

# ---  account Me: password added ---------------------------------------------

add account-me-pw-added "$ENTRY_R"

# --- setup -------------------------------------------------------------------

accounts setup "long 201 23" "tap 152 141"

# --- setup time --------------------------------------------------------------

accounts setup-time "time 1716272769" \
    "long 201 23" "tap 152 141" "tap 93 119"

# --- setup storage -----------------------------------------------------------

accounts setup-storage "long 201 23" "tap 152 141" "tap 81 214"

# --- delete account (swipe not started) --------------------------------------

accounts account-demo-top-delete "tap 86 67" "long 201 23" \
    "tap 193 142"

# --- delete account (yellow) -------------------------------------------------

accounts account-demo-top-delete-yellow "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 116 200"

# --- delete account (green) --------------------------------------------------

accounts account-demo-top-delete-green "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 200"

# --- deleted account ---------------------------------------------------------

accounts account-demo-deleted "tap 86 67" "long 201 23" \
    "tap 193 142" "drag 53 189 180 200"

# --- delete account (red) ---------------------------------------------------

accounts account-demo-top-delete-red "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 150"

# --- account overlay (password field) ---------------------------------------

accounts account-demo-pw-over "tap 86 67" "long 199 119"

# --- delete field (swipe not started) ---------------------------------------

accounts account-demo-pw-delete "tap 86 67" "long 199 119" \
    "tap 153 174"

# --- deleted field  ----------------------------------------------------------

accounts account-demo-pw-deleted "tap 86 67" "long 199 119" \
    "tap 153 174" "drag 53 189 180 200"

# --- account overlay (bottom) ------------------------------------------------

accounts account-demo-bottom-over "tap 86 67" "long 107 194"

# --- account edit password (Geheimx) -----------------------------------------

# Adding an "x" to "Geheim" makes the text too long for centering, but doesn't
# yet require cutting any off-screen part.

accounts account-demo-pw-geheimx "tap 86 67" "long 199 119" \
    "tap 88 172" "$ENTRY_9" "$ENTRY_5"

# --- account show secret (HOTP) ----------------------------------------------

# The secret is a long base32 string that gets cut off at the screen edge.

accounts account-hotp-secret "drag 158 243 159 170" "tap 50 221" \
    "long 75 66" "tap 91 173"

# --- account with "comment" field --------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw",
    "comment":"comment" } ]
EOF

accounts account-comment "tap 86 67"

# --- account with 2nd password field -----------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts account-pw2 "tap 86 67"

# --- account with 2nd password and TOTP --------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp "time 0" "tap 86 67"

# --- account with 2nd password and TOTP, scrolled up, at 5 s -----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-5 \
    "time 5" "tap 86 67" "drag 158 243 159 130"

# --- account with 2nd password and TOTP, scrolled up, at 20 s ----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-20 \
    "time 20" "tap 86 67" "drag 158 243 159 130"

# --- account with 2nd password and TOTP, scrolled up, at 31 s ----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-31 \
    "time 31" "tap 86 67" "drag 158 243 159 130"

# --- delete 2nd password field -----------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts delete-pw2 "tap 86 67" \
    "long 119 233" "tap 154 173" "drag 56 190 183 191"

# --- delete 1st password field -----------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts delete-pw1 "tap 86 67" \
    "long 119 177" "tap 154 173" "drag 56 190 183 191"

# --- setup version -----------------------------------------------------------

accounts setup-version "long 201 23" "tap 152 141" static "tap 81 268"

# --- setup R&D ---------------------------------------------------------------

accounts setup-rd "long 201 23" "tap 152 141" \
    "drag 66 200 68 103" "tap 68 219"

# --- Move from ---------------------------------------------------------------

accounts move-from "long 200 72" "$ACCOUNTS_MOVE"

# --- Moving -----------------------------------------------------------------

add moving "long 114 166"

# --- Moved -------------------------------------------------------------------

save
add moved "$ACCOUNTS_MOVE"
restore

# --- Move cancel -------------------------------------------------------------

add move-cancel "$ACCOUNTS_CANCEL_MOVE"

# === Edit entry name (demo) =================================================

accounts entry-edit "tap 86 67" "long 201 23" "tap 116 141"

# --- Edit entry name (remove "emo") ------------------------------------------

add entry-edit-d "$ENTRY_L" "$ENTRY_L" "$ENTRY_L"

# --- Edit entry name ("dummy1", duplicate) -----------------------------------

add entry-edit-dummy1 \
    "$ENTRY_8" "$ENTRY_5" \
    "$ENTRY_6" "$ENTRY_4" "$ENTRY_6" "$ENTRY_4" \
    "$ENTRY_9" "$ENTRY_6" \
    "$ENTRY_1" "$ENTRY_0"

# --- Edit entry name ("dummy12", then back, duplicate) -----------------------

save
add entry-edit-dummy12 \
    "$ENTRY_2" "$ENTRY_0" \
    "$ENTRY_L"
restore

# --- Edit entry name ("dummy1xxx" ) ------------------------------------------

add entry-edit-dummy1xxx \
    "$ENTRY_9" "$ENTRY_5" "$ENTRY_9" "$ENTRY_5" "$ENTRY_9" "$ENTRY_5"

# --- Edit entry name ("dummy1xxx123456" ) ------------------------------------

add entry-edit-6 \
    "$ENTRY_1" "$ENTRY_0" "$ENTRY_2" "$ENTRY_0" "$ENTRY_3" "$ENTRY_0" \
    "$ENTRY_4" "$ENTRY_0" "$ENTRY_5" "$ENTRY_0" "$ENTRY_6" "$ENTRY_0"

# --- Edit entry name ("dummy1xxx1234567", maximum length) --------------------

add entry-edit-7 "$ENTRY_7" "$ENTRY_0"

# --- Edit entry name ("dummy1xxx1234567", no more) ---------------------------

add entry-edit-8 "$ENTRY_8" "$ENTRY_0"

# === long entry, show ========================================================

accounts long-show "tap 32 270"

# --- long entry, scroll to the left ------------------------------------------

add long-left "drag 220 120 100 120"
add long-left2 "drag 220 120 110 121"
add long-left3 "drag 240 120 100 120"

# =============================================================================

$sourced || atend
