#!/usr/bin/python3
#
# accenc.py - Encode an account database from a JSON file
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#


import sys, os, struct, json, base64, re
import nacl
from nacl.secret import SecretBox
from nacl.public import PublicKey, PrivateKey, Box

BLOCK_SIZE = 1024
NONCE_SIZE = 24
NONCE_PAD = 8
PAYLOAD_SIZE = 956
HASH_SIZE = 20
HASH_PAD = 12

STORAGE_BLOCKS = 2048	# total number of blocks, including pad block
PAD_BLOCKS = 8		# blocks reserved for pads
RESERVED_BLOCKS = PAD_BLOCKS


keys = ( "id", "prev", "user", "email", "pw", "hotp_secret", "hotp_counter",
    "totp_secret", "comment", "pw2" )

debug = False


def encode(key, code, v):
	if key == "id" or key == "prev" or key == "user" or key == "email" or \
	    key == "pw" or key == "pw2" or key == "comment":
		return struct.pack("BB", code, len(v)) + v.encode()
	if key == "hotp_secret" or key == "totp_secret":
		s = base64.b32decode(v)
		return struct.pack("BB", code, len(s)) + s
	if key == "hotp_counter":
		return struct.pack("<BBQ", code, 8, int(v))
	raise Exception("unknown key " + key)


def write_old(b):
	sys.stdout.buffer.write(os.urandom(NONCE_SIZE) + b'\000' * NONCE_PAD +
	    struct.pack("<BBH", 4, 0, 0) +
	    b + b'\000' * (PAYLOAD_SIZE - len(b)) +
	    b'\x55' * (HASH_SIZE + HASH_PAD))


# Adapted from anelok/crypter/account_db.py

def write_new(data, writer, readers):
	rk = nacl.utils.random(SecretBox.KEY_SIZE)
	nonce = nacl.utils.random(Box.NONCE_SIZE)

	if debug:
		print("Wpk", bytes(writer.public_key).hex(), file = sys.stderr)
		print("Nonce", nonce.hex(), file = sys.stderr)
		print("RK", rk.hex(), file = sys.stderr)
		print("Readers", len(readers), file = sys.stderr)
	blob = bytes(writer.public_key) + nonce

	i = 0
	for rd in readers:
		nonce2 = bytearray(nonce)
		i += 1
		nonce2[0] ^= i
		shared = bytes(Box(writer, rd))
		box = SecretBox(shared)
		ek = box.encrypt(rk, bytes(nonce2))[Box.NONCE_SIZE +
		    SecretBox.MACBYTES:]
		blob += ek
		if debug:
			print("\tShared", bytes(shared).hex(),
			    file = sys.stderr)
			print("\tRpk", bytes(rd).hex(), file = sys.stderr)
			print("\tEK", ek.hex(), file = sys.stderr)

	space = BLOCK_SIZE - len(blob) - SecretBox.MACBYTES
	assert space >= len(data)

	box = SecretBox(rk)
	data += b'\000' * (space - len(data))
	blob += box.encrypt(data, nonce)[Box.NONCE_SIZE:]
	assert len(blob) == BLOCK_SIZE

	sys.stdout.buffer.write(blob)


if len(sys.argv) < 2:
        print("usage:", sys.argv[0], "db.json [writer [reader ...]]",
	    file = sys.stderr)
        sys.exit(1)

s = ""
with open(sys.argv[1]) as file:
	for line in file:
		if not re.match("^\s*#", line):
			s += line
db = json.loads(s)

if len(sys.argv) == 2:
	writer = None
else:
	writer = PrivateKey(base64.b32decode(sys.argv[2]))
	if len(sys.argv) == 3:
		readers = [ writer.public_key ]
	else:
		readers = map(lambda x: PublicKey(base64.b32decode(x)),
		    sys.argv[3:])

sys.stdout.buffer.write(b'\xff' * RESERVED_BLOCKS * BLOCK_SIZE)

for e in db:
	b = b''
	i = 1
	for key in keys:
		if key in e:
			b += encode(key, i, e[key])
		i += 1
	if writer is None:
		write_old(b)
	else:
		write_new(struct.pack("<BBH", 4, 0, 0) + b,
		    writer, [ writer.public_key ])
	
sys.stdout.buffer.write(
    (b'\xff' * (STORAGE_BLOCKS - RESERVED_BLOCKS - len(db)) * BLOCK_SIZE))
