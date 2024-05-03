#!/usr/bin/perl
#
# mksintab.pl - Make a table of sine values from 0-45 degrees (scaled by 255)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

$pi = 4 * atan2(1, 1);
for ($i = 0; $i <= 45; $i++) {
	$f = $i / 180 * $pi;
	$t = sin($f) / cos($f) + 1e-6;	# make sure we reach 255
	printf("\t%u,\t// [%u] %5.3f\n", 255 * $t, $i, $t);
}
