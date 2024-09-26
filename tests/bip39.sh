#!/bin/sh
#
# bip39.sh - Test the BIP39 reference vectors
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

VECTORS=../lib/bip39/vectors.json


usage()
{
	echo "usage: $0 [-v] [-x]" 1>&2
	exit 1
}


verbose=false
while [ "$1" ]; do
	case "$1" in
	-v)	verbose=true
		shift;;
	-x)	set -x
		shift;;
	*)	usage;;
	esac
done

jq -r '.english[] | .[0]+" "+.[1]' $VECTORS | while read l; do
	set - $l
	hex=$1
	shift

	$verbose && echo "$hex -> $res" 1>&2
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

	$verbose && echo "$* -> $hex" 1>&2
	res=`../sim -q -C "bip39 decode $*"` || exit
	if [ "$hex" != "$res" ]; then
		cat <<EOF 2>&2
Vector mismatch:
Input:    $*
Output:   $res
Expected: $hex
EOF
		exit 1
	fi
done
