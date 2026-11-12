"""The seal and the scrambling of a model blob, on the PC side. The FPGA does
the same in rtl/security/ (cmac_stream.v, ctr_stream.v).

Keys, all AES-128, worked out from one master key that never leaves the PC:
    stamp key       = AES-CMAC(master, device ID as 4 bytes little endian)
    scrambling key  = AES-CMAC(master, "ENC" + device ID as 4 bytes little endian)
Two different keys, so the stamp and the scrambling never share one. The
stamp key is the one sign_blob.py has always used, so old stamps stay valid.

Seal (MAC) = AES-CMAC(stamp key, every byte before the MAC), the CRC included.

Scrambling (AES-CTR): blob bytes 176 up to the CRC (self tests, key table,
nodes) are xored with AES(scrambling key, IV || block number), the IV being
12 random bytes at header offset 40, the block number 32 bits big endian from
0. Header byte 6 bit 0 (Flags) says the blob is scrambled. The CRC and the seal
are worked out on the scrambled bytes ("encrypt then MAC").
"""
import os
import struct
import zlib

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.cmac import CMAC

import blob_format as bf

# Simulation only. It is printed here, so it protects nothing: the testbenches
# and build/test_blobs use it, a real board never does (updatable_top.v takes
# its keys from keys/device_key.vh, made by make_device_key.py from your key).
TEST_MASTER_KEY = b"PUBLIC TEST KEY!"

OFF_IV = 40
IV_SIZE = 12
OFF_SCRAMBLED = bf.OFF_SELFTEST


def cmac(key, msg):
    c = CMAC(algorithms.AES(key))
    c.update(msg)
    return c.finalize()


def aes_block(key, block):
    e = Cipher(algorithms.AES(key), modes.ECB()).encryptor()
    return e.update(block) + e.finalize()


def mac_key(master, device_id):
    return cmac(master, struct.pack("<I", device_id))


def enc_key(master, device_id):
    return cmac(master, b"ENC" + struct.pack("<I", device_id))


def keystream(key, iv, n):
    out = bytearray()
    blk = 0
    while len(out) < n:
        out += aes_block(key, bytes(iv) + struct.pack(">I", blk))
        blk += 1
    return bytes(out[:n])


def _xor_region(b, key):
    end = len(b) - bf.TRAILER_SIZE
    ks = keystream(key, b[OFF_IV:OFF_IV + IV_SIZE], end - OFF_SCRAMBLED)
    for i, k in enumerate(ks):
        b[OFF_SCRAMBLED + i] ^= k


def seal(blob, master, device_id):
    """Put the CRC and the stamp on a blob (scrambled or not)."""
    b = bytearray(blob)
    n = len(b)
    struct.pack_into("<I", b, n - bf.TRAILER_SIZE, zlib.crc32(bytes(b[:n - bf.TRAILER_SIZE])))
    b[n - 16:] = cmac(mac_key(master, device_id), bytes(b[:n - 16]))
    return bytes(b)


def scramble(blob, master, device_id, iv=None):
    """Scramble a plain blob: set the flag, write a fresh random IV, xor the
    region. Seal it afterwards. A repeated IV under the same key would leak the
    rulebook, so the IV is always random unless a test passes one."""
    b = bytearray(blob)
    flags = struct.unpack_from("<H", b, 6)[0]
    if flags & bf.FLAG_ENCRYPTED:
        raise ValueError("already scrambled")
    struct.pack_into("<H", b, 6, flags | bf.FLAG_ENCRYPTED)
    b[OFF_IV:OFF_IV + IV_SIZE] = os.urandom(IV_SIZE) if iv is None else iv
    _xor_region(b, enc_key(master, device_id))
    return bytes(b)


def unscramble(blob, master, device_id):
    """The plain blob behind a scrambled one: flag cleared, CRC recomputed,
    MAC zero (so bf.parse can check it). The IV is left in the header."""
    b = bytearray(blob)
    flags = struct.unpack_from("<H", b, 6)[0]
    if not flags & bf.FLAG_ENCRYPTED:
        raise ValueError("not scrambled")
    _xor_region(b, enc_key(master, device_id))
    struct.pack_into("<H", b, 6, flags & ~bf.FLAG_ENCRYPTED)
    n = len(b)
    struct.pack_into("<I", b, n - bf.TRAILER_SIZE, zlib.crc32(bytes(b[:n - bf.TRAILER_SIZE])))
    b[n - 16:] = bytes(16)
    return bytes(b)


def verify(blob, master, device_id):
    return cmac(mac_key(master, device_id), bytes(blob[:-16])) == bytes(blob[-16:])


def verilog_hex(key):
    """A key as a Verilog constant: byte 0 is the most significant (AES order)."""
    return "128'h" + key.hex()
