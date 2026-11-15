"""Blobs for the update tests: good ones with chosen versions, and one bad blob
for each fault the FPGA must reject. Every blob carries a correct CRC and a
correct stamp (MAC, with the public test key, blob_crypto.TEST_MASTER_KEY)
unless that is the fault under test, so the check under test is the one that
fires. COMMIT checks in this order: header, protocol, device, version, length,
CRC, the stamp, then node ranges and the self test (the two that look at
unscrambled bytes, so only a stamped blob gets an answer about them).

    py -3.13 make_test_blobs.py      (after export_blob.py for 5, 6 and 8 bytes)

Reads build/blobs/pN/model.bin, writes build/test_blobs/<name>.hex and
manifest.txt with each blob's length and CRC for the testbenches.
"""
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blob_crypto as bc  # noqa: E402
import blob_format as bf  # noqa: E402
import paths  # noqa: E402

DEVICE = 1
KEY = bc.TEST_MASTER_KEY
OTHER_KEY = b"another test key"          # a different master key: a wrong stamp


def load(p):
    with open(os.path.join(paths.BUILD, "blobs", "p%d" % p, "model.bin"), "rb") as f:
        return bytearray(f.read())


def device_of(b):
    return struct.unpack_from("<I", b, 20)[0]


def seal(b, key=KEY):
    """CRC and stamp after a change, for the device the header names."""
    return bytearray(bc.seal(bytes(b), key, device_of(b)))


def fix_crc(b):
    """A correct CRC, the stamp left as it was: anyone can do this."""
    x = bytearray(b)
    n = len(x)
    struct.pack_into("<I", x, n - bf.TRAILER_SIZE, zlib.crc32(bytes(x[:n - bf.TRAILER_SIZE])))
    return x


def field32(b, off, v):
    struct.pack_into("<I", b, off, v)


def plain(p, version):
    b = load(p)
    field32(b, 16, version)
    field32(b, 20, DEVICE)
    return b


def good(p, version):
    return seal(plain(p, version))


def scrambled(p, version, iv):
    return seal(bytearray(bc.scramble(bytes(plain(p, version)), KEY, DEVICE, iv)))


def attacker_edit(b):
    """Change one threshold by 1, in a node whose change leaves every self test
    vote as it was and every field in range, then fix the CRC but not the
    stamp: what someone without the key would do. Such a blob passes every
    check before the stamp, so only the stamp can catch it."""
    h = bf.parse(bytes(seal(b)))
    tests = h["selftests"]
    X = [t[0] for t in tests]
    want = [t[1] for t in tests]
    trees = h["trees"]
    off = bf.OFF_NODES
    for t, levels in enumerate(trees):
        for l, level in enumerate(levels):
            for i, (feat, thr, left, right) in enumerate(level):
                if thr < 0xFFFF:
                    level[i] = (feat, thr + 1, left, right)
                    votes = list(bf.forest_votes(trees, X, h["n_classes"]))
                    level[i] = (feat, thr, left, right)
                    if votes == want:
                        x = bytearray(b)
                        x[off:off + bf.NODE_SIZE] = bf.pack_node(feat, thr + 1, left, right)
                        n = len(x)
                        x = seal(x)
                        x[n - 16:] = b[n - 16:] if len(b) == n else bytes(16)
                        return x
                off += bf.NODE_SIZE
    raise SystemExit("no node can change without moving a self test vote")


def main():
    out = os.path.join(paths.BUILD, "test_blobs")
    os.makedirs(out, exist_ok=True)
    blobs = {}

    blobs["v2_p8"] = good(8, 2)
    blobs["v3_p5"] = good(5, 3)
    blobs["v4_p6"] = good(6, 4)
    blobs["v5_p8"] = good(8, 5)
    for p in (5, 6, 8):                    # for the dataset run over SPI
        blobs["ds_p%d" % p] = good(p, 2)
        blobs["ds_p%d_enc" % p] = scrambled(p, 2, bytes(range(p, p + 12)))

    base = 9
    x = good(5, base); x[bf.OFF_NODES + 2] ^= 0x10
    blobs["bad_crc_body"] = x              # threshold flipped, CRC not fixed

    blobs["truncated"] = good(5, base)[:2000]

    x = good(5, base); x[bf.OFF_NODES + 4] = 0x0F
    blobs["bad_feature"] = seal(x)         # feature 15 of 10

    x = good(5, base); x[bf.OFF_NODES + 1] = 0x7F
    blobs["bad_child"] = seal(x)           # child 127: past the next level

    blobs["lower_version"] = good(5, 1)

    x = good(5, base); field32(x, 20, DEVICE + 1)
    blobs["wrong_device"] = seal(x)        # stamped for board 2, header says board 2

    x = good(5, base); struct.pack_into("<H", x, 24, 2)
    blobs["wrong_protocol"] = seal(x)

    x = good(5, base); x[bf.OFF_SELFTEST + 32] = (x[bf.OFF_SELFTEST + 32] + 1) % 5
    blobs["bad_selftest"] = seal(x)

    x = good(5, base); field32(x, 20, bf.DEVICE_ANY)
    blobs["wildcard_device"] = seal(x)     # only the boot ROM may use the wildcard

    x = good(5, base); x[30] = bf.MIN_FEAT_BYTES - 1
    blobs["few_bytes"] = seal(x)           # reads 4 data bytes: the allow list needs 5

    # ---- the stamp (Stage 5). Each passes every check before the stamp.
    x = good(5, base); x[-16:] = bytes(16)
    blobs["mac_missing"] = x               # never stamped
    x = good(5, base); x[-16] ^= 0x01
    blobs["mac_bitflip"] = x               # one stamp bit wrong
    blobs["attacker_edit"] = attacker_edit(good(5, base))
    blobs["other_key"] = seal(plain(5, base), OTHER_KEY)     # stamped with another master key
    blobs["other_key_v5"] = seal(plain(8, 5), OTHER_KEY)     # for the 3 strike test (floor 5)
    x = plain(8, 5); x = seal(x); x[-16:] = bytes(16)
    blobs["unsigned_v5_p8"] = x            # written straight into flash: must not boot

    # ---- no answers about the content without a stamp (the probing hole).
    # A node's feature byte set to 15 (out of range), CRC fixed, the old stamp
    # kept: what someone without the key can make. It must get E_MAC, not E_RANGE.
    x = good(5, base); x[bf.OFF_NODES + 4] = 0x0F
    blobs["bad_feature_nokey"] = fix_crc(x)
    # The same on a scrambled blob, by flipping the scrambled byte (CTR: the
    # plain byte flips with it). With the stamp: E_RANGE, so the fault is real.
    enc = scrambled(5, base, bytes(range(40, 52)))
    p_feat = plain(5, base)[bf.OFF_NODES + 4]
    x = bytearray(enc); x[bf.OFF_NODES + 4] ^= p_feat ^ 0x0F
    blobs["enc_bad_node_nokey"] = fix_crc(x)
    blobs["enc_bad_node"] = seal(x)

    # ---- scrambling
    blobs["enc_v6_p6"] = scrambled(6, 6, bytes(range(100, 112)))
    blobs["plain_v6_p6"] = good(6, 6)      # the same model, for the flash comparison
    x = bytearray(scrambled(5, base, bytes(range(12))))
    struct.pack_into("<H", x, 6, 0)        # flag cleared: the FPGA reads noise as nodes
    blobs["enc_flag_cleared"] = seal(x)    # stamped, so it gets past the stamp and fails after

    with open(os.path.join(out, "manifest.txt"), "w") as m:
        for name, b in blobs.items():
            with open(os.path.join(out, name + ".hex"), "w") as f:
                f.write("".join("%02x\n" % v for v in b))
            n = len(b)
            full = n if name != "truncated" else struct.unpack_from("<I", b, 8)[0]
            crc = struct.unpack_from("<I", b, n - bf.TRAILER_SIZE)[0] if n == full else 0
            m.write("%s %d %08x\n" % (name, n, crc))
    print("Wrote %d blobs to %s" % (len(blobs), out))


if __name__ == "__main__":
    main()
