"""Blobs for the update tests: good ones with chosen versions, and one bad blob
for each fault the FPGA must reject. Every bad blob has a correct CRC unless
the fault is the CRC itself, so the check under test is the one that fires.

    py -3.13 make_test_blobs.py      (after export_blob.py for 5, 6 and 8 bytes)

Reads build/blobs/pN/model.bin, writes build/test_blobs/<name>.hex and
crc32.txt with the blob's CRC and length for the testbench.
"""
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blob_format as bf  # noqa: E402
import paths  # noqa: E402

DEVICE = 1


def load(p):
    with open(os.path.join(paths.BUILD, "blobs", "p%d" % p, "model.bin"), "rb") as f:
        return bytearray(f.read())


def seal(b):
    """Put the right CRC back after a change."""
    n = len(b)
    struct.pack_into("<I", b, n - bf.TRAILER_SIZE, zlib.crc32(bytes(b[:n - bf.TRAILER_SIZE])))
    return b


def field32(b, off, v):
    struct.pack_into("<I", b, off, v)


def good(p, version):
    b = load(p)
    field32(b, 16, version)
    field32(b, 20, DEVICE)
    return seal(b)


def first_node(b):
    return bf.OFF_NODES


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
    blobs["wrong_device"] = seal(x)

    x = good(5, base); struct.pack_into("<H", x, 24, 2)
    blobs["wrong_protocol"] = seal(x)

    x = good(5, base); x[bf.OFF_SELFTEST + 32] = (x[bf.OFF_SELFTEST + 32] + 1) % 5
    blobs["bad_selftest"] = seal(x)

    x = good(5, base); field32(x, 20, bf.DEVICE_ANY)
    blobs["wildcard_device"] = seal(x)     # only the boot ROM may use the wildcard

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
