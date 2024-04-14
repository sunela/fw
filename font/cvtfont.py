#!/usr/bin/python3
#
# cvtfont.py - Generate RLE-encoded bitmap fonts from TTF (using otf2bdf)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

#
# TTF to BDF conversion:
# https://superuser.com/a/336550
#
# BDF file format:
# https://en.wikipedia.org/wiki/Glyph_Bitmap_Distribution_Format
#
# Illustration of font metrics:
# https://imagemagick.org/Usage/text/#font_info
#

import sys, subprocess, re


def preamble():
	print("const struct font " + name + " = {")
#	print("\tascent:\t\t" + ascent + ",")
#	print("\tdescent:\t" + descent + ",")
	print("\tw:\t\t" +  fbox[0] + ",")
	print("\th:\t\t" +  fbox[1] + ",")
	print("\tox:\t\t" +  fbox[2] + ",")
	print("\toy:\t\t" +  fbox[3] + ",")
	print("\tn_chars:\t" +  chars + ",")
	print("\tchars: {")


def epilogue():
	print("\t}")
	print("};")


def character():
	global best

#	print(code, bbx[0], bbx[1], bbx[2], bbx[3], advance, start)
	print("\t{ code: 0x" + code + ", w: " + bbx[0] + ", h: " + bbx[1] +
	    ", ox: " + bbx[2] + ", oy: " + bbx[3] + ", advance: " + advance +
	    ",");

	if len(data) == 0:
		print("\t  bits: 0 },");
		return

	print("// RAW", best)
	print("// RLE", data)
	# find best encoding size

	best_bits = 1
	for bits in range(2, 8):
		temp = []
		for span in data:
			span_len = span
			while span_len > (1 << bits) - 1:
				temp.append((1 << bits) - 1)
				span_len -= (1 << bits) - 1
			temp.append(span_len - 1)
		if len(temp) * bits < len(best) * best_bits:
			best_bits = bits
			best = temp

#	print("BEST", best_bits, best)
	print("\t  bits: " + str(best_bits) + ", start: " + str(start) + ",")
	print("\t  data: (const uint8_t []) { ", end = "")

	v = 0
	length = 0
	for span in best:
		v += span << length
		length += best_bits
#		print("X span", span, "v", v, "length", length)
		if length > 8:
			print(str(v & 255) + ", ", end = "")
			v >>= 8
			length -= 8
	print(v, "} },")


if len(sys.argv) < 4:
	print("usage: ttf-font size name [from[-to] ...]", file = sys.stderr)
	sys.exit(1)

font = sys.argv[1]
size = sys.argv[2]
name = sys.argv[3]
select = sys.argv[4:]

if len(select) == 0:
	subset = ""
else:
	subset = "-l "
	for arg in select:
		print("ARG", arg, file = sys.stderr)
		m = re.match("^([0-9A-Fa-f]+)$", arg)
		if m is not None:
			subset += m.group(1) + " "
			continue
		m = re.match("^([0-9A-Fa-f]+)-([0-9A-Fa-f]+)$", arg)
		if m is not None:
			subset += m.group(1) + "_" + m.group(2) + " "
			continue
		raise Exception("invalid subset")

#print("otf2bdf " + font + " -p " + size)
(exitcode, output) = subprocess.getstatusoutput(
    "otf2bdf " + subset + font + " -p " + size)
# @@@ otf2bdf returns 8 on success. why ???
#if exitcode != 0:
#	sys.exit(exitcode)

first = True
ascent = None
descent = None
fbox = None
chars = None

code = None
advance = None
bbx = None
start = None
data = None

for line in output.splitlines():
#	print("L", line)
	m = re.match(r"^FONTBOUNDINGBOX (\d+) (\d+) (-?\d+) (-?\d+)", line)
	if m is not None:
		fbox = (m.group(1), m.group(2), m.group(3), m.group(4))
		continue
#	m = re.match(r"^FONT_ASCENT (\d+)", line)
#	if m is not None:
#		ascent = m.group(1)
#		continue
#	m = re.match(r"^FONT_DESCENT (\d+)", line)
#	if m is not None:
#		descent = m.group(1)
#		continue
	m = re.match(r"^CHARS (\d+)", line)
	if m is not None:
		chars = m.group(1)
		continue
	m = re.match(r"^STARTCHAR ([0-aA-F]{4})", line)
	if m is not None:
		if first:
			preamble()
			first = False
		code = m.group(1)
		print("// CODE", code)
		continue
	m = re.match(r"^DWIDTH (\d+)", line)
	if m is not None:
		advance = m.group(1)
		continue
	m = re.match(r"^BBX (\d+) (\d+) (-?\d+) (-?\d+)", line)
	if m is not None:
		bbx = (m.group(1), m.group(2), m.group(3), m.group(4))
		continue
	m = re.match(r"^BITMAP", line)
	if m is not None:
		data = []
		continue
	m = re.match(r"^ENDCHAR", line)
	if m is not None:
		if data is None:
			raise Exception("ENDCHAR without data")
		character()
		code = None
		advance = None
		bbx = None
		start = None
		data = None
		continue
	if data is None:
		continue
	m = re.match(r"([0-9A-F]{2})+", line)
	if m is None:
		raise Exception("bad data")

	# convert line to run-length encoding

	if start is None:
		start = int(line[0], 16) >> 3
		data = [ 0 ]
		last = start
		best = []

	print("// LINE", line)
	for i in range(0, len(line) // 2):
		v = int(line[2 * i:2 * i + 2], 16)
		print("// V", v, "last", last)
		for j in range(0, 8):
			if i * 8 + j >= int(bbx[0]):
				break
			if (v >> (7 - j)) & 1 == last:
				data[-1] += 1
			else:
				data.append(1)
				last = 1 - last
			best.append(last)
print("} };");
