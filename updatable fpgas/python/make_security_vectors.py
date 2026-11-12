"""Reference answers for the security circuits, made with Python's
`cryptography` library (the same one sign_blob.py uses), so the RTL is checked
against an independent AES, not against itself.

    py -3.13 make_security_vectors.py

Writes build/sec_vectors/:
    aes.txt    key plaintext ciphertext, one per line: FIPS-197 appendix C.1,
               then 1,000 random blocks with random keys
    cmac.txt   key length tag, then the message as one hex line: the RFC 4493
               examples (16, 40 and 64 bytes; the 0 byte example is listed
               apart because a byte stream of length 0 never ends a block),
               then every real blob length from build/blobs, then random lengths
    ctr.txt    key iv block keystream: blocks 0 to 2 for two keys and IVs, as
               sign_blob.py scrambles (IV in header bytes 40-51, then a 32 bit
               big endian block number)
"""
import os
import random
import sys

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.cmac import CMAC

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import paths  # noqa: E402


def aes(key, block):
    e = Cipher(algorithms.AES(key), modes.ECB()).encryptor()
    return e.update(block) + e.finalize()


def cmac(key, msg):
    c = CMAC(algorithms.AES(key))
    c.update(msg)
    return c.finalize()


RFC_KEY = bytes.fromhex("2b7e151628aed2a6abf7158809cf4f3c")
RFC_MSG = bytes.fromhex(
    "6bc1bee22e409f96e93d7e117393172a"
    "ae2d8a571e03ac9c9eb76fac45af8e51"
    "30c81c46a35ce411e5fbc1191a0a52ef"
    "f69f2445df4f9b17ad2b417be66c3710")
RFC_TAGS = {16: "070a16b46b4d4144f79bdd9dd04a287c",
            40: "dfa66747de9ae63030ca32611497c827",
            64: "51f0bebf7e3b9d92fc49741779363cfe"}


def main():
    out = os.path.join(paths.BUILD, "sec_vectors")
    os.makedirs(out, exist_ok=True)
    rnd = random.Random(20261112)

    with open(os.path.join(out, "aes.txt"), "w") as f:
        k = bytes(range(16))
        p = bytes.fromhex("00112233445566778899aabbccddeeff")
        c = aes(k, p)
        assert c.hex() == "69c4e0d86a7b0430d8cdb78070b4c55a"          # FIPS-197 C.1
        f.write("%s %s %s\n" % (k.hex(), p.hex(), c.hex()))
        for _ in range(1000):
            k = bytes(rnd.getrandbits(8) for _ in range(16))
            p = bytes(rnd.getrandbits(8) for _ in range(16))
            f.write("%s %s %s\n" % (k.hex(), p.hex(), aes(k, p).hex()))

    cases = []
    for n, tag in RFC_TAGS.items():
        assert cmac(RFC_KEY, RFC_MSG[:n]).hex() == tag                 # RFC 4493 section 4
        cases.append((RFC_KEY, RFC_MSG[:n]))
    assert cmac(RFC_KEY, b"").hex() == "bb1d6929e95937287fa37d129b756746"
    lengths = []
    for p in (5, 6, 8):
        b = os.path.join(paths.BUILD, "blobs", "p%d" % p, "model.bin")
        if os.path.exists(b):
            lengths.append(os.path.getsize(b) - 16)                   # the seal covers all but the MAC
    lengths += [17, 31, 32, 33, 47, 48, 49, 100, 255, 256, 1000]
    for n in lengths:
        k = bytes(rnd.getrandbits(8) for _ in range(16))
        cases.append((k, bytes(rnd.getrandbits(8) for _ in range(n))))
    with open(os.path.join(out, "cmac.txt"), "w") as f:
        for k, m in cases:
            f.write("%s %d %s\n%s\n" % (k.hex(), len(m), cmac(k, m).hex(), m.hex() if m else "00"))

    with open(os.path.join(out, "ctr.txt"), "w") as f:
        for _ in range(2):
            k = bytes(rnd.getrandbits(8) for _ in range(16))
            iv = bytes(rnd.getrandbits(8) for _ in range(12))
            for blk in range(3):
                f.write("%s %s %d %s\n" % (k.hex(), iv.hex(), blk, aes(k, iv + blk.to_bytes(4, "big")).hex()))

    print("Wrote %d AES, %d CMAC and 6 CTR vectors to %s" % (1001, len(cases), out))
    print("CMAC lengths: %s (RFC 4493 example 1, length 0, is checked in Python only)"
          % ", ".join(str(len(m)) for _, m in cases))


if __name__ == "__main__":
    main()
