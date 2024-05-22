#!/usr/bin/python3
#
# accenc.py - Encode an account database from a JSON file
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#


import sys, os, struct, json, base64, re


BLOCK_SIZE = 1024
NONCE_SIZE = 24
NONCE_PAD = 8
PAYLOAD_SIZE = 956
HASH_SIZE = 20
HASH_PAD = 12

STORAGE_BLOCKS = 2048


keys = ( "id", "prev", "user", "email", "pw", "hotp_secret", "hotp_counter",
    "totp_secret", "comment" )


def encode(key, code, v):
	if key == "id" or key == "prev" or key == "user" or key == "email" or \
	    key == "pw" or key == "comment":
		return struct.pack("BB", code, len(v)) + v.encode()
	if key == "hotp_secret" or key == "totp_secret":
		s = base64.b32decode(v)
		return struct.pack("BB", code, len(s)) + s
	if key == "hotp_counter":
		return struct.pack("<BBQ", code, 8, int(v))
	raise Exception("unknown key " + key)


if len(sys.argv) != 2:
        print("usage:", sys.argv[0], "db.json", file = sys.stderr)
        sys.exit(1)

s = ""
with open(sys.argv[1]) as file:
	for line in file:
		if not re.match("^\s*#", line):
			s += line
db = json.loads(s)

for e in db:
	b = b''
	i = 1
	for key in keys:
		if key in e:
			b += encode(key, i, e[key])
		i += 1
	sys.stdout.buffer.write(os.urandom(NONCE_SIZE) + b'\000' * NONCE_PAD +
	    struct.pack("<BBH", 4, 0, 0) +
	    b + b'\000' * (PAYLOAD_SIZE - len(b)) +
	    b'\x55' * (HASH_SIZE + HASH_PAD))
sys.stdout.buffer.write((b'\xff' * (STORAGE_BLOCKS - len(db)) * BLOCK_SIZE))
