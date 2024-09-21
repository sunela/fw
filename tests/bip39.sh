#!/bin/sh
#
# bip39.sh - Test the BIP39 reference vectors
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

VECTORS=../lib/bip39/vectors.json

jq -r '.english[] | .[0]+" "+.[1]' $VECTORS | while read l; do
	set - $l
	hex=$1
	shift
	res=`../sim -q -C "bip39 encode $hex"` || exit
	if [ "$*" != "$res" ]; then
		cat <<EOF 2>&2
Vector mismatch:
Input:    $hex
Output:   $res
Expected: $*
EOF
		exit 1
	fi
done
