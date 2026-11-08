"""Sign a model blob for one board. Runs on your PC only; the STM32 only ever
carries the result.

    py -3.13 sign_blob.py build/blobs/p5/model.bin --device-id 1 --version 2 \\
        --key keys/master.key --out build/signed/p5_dev1_v2.bin

    py -3.13 sign_blob.py --new-key keys/master.key     makes a random master key

Writes the device ID and version into the blob, recomputes its CRC and fills the
MAC field. Keys:
    device key = AES-CMAC(master key, device ID)
    MAC        = AES-CMAC(device key, every byte of the blob before the MAC)
A blob signed for one board fails on another. Keep keys/ out of git (it is
ignored) and off the STM32.

The FPGA does not check the MAC yet: that is Phase 2 (docs/SECURITY_DESIGN.md).
Today's CRC only catches mistakes, it does not stop an attacker.
"""
import argparse
import os
import struct
import sys
import zlib

from cryptography.hazmat.primitives.cmac import CMAC
from cryptography.hazmat.primitives.ciphers.algorithms import AES

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blob_format as bf  # noqa: E402


def cmac(key, msg):
    c = CMAC(AES(key))
    c.update(msg)
    return c.finalize()


def read_key(path):
    with open(path, "rb") as f:
        raw = f.read()
    if len(raw) != 16:
        try:
            raw = bytes.fromhex(raw.decode().strip())
        except ValueError:
            raise SystemExit("%s is not a 16 byte AES key" % path)
    if len(raw) != 16:
        raise SystemExit("%s is not a 16 byte AES key" % path)
    return raw


def device_key(master, device_id):
    return cmac(master, struct.pack("<I", device_id))


def sign(blob, device_id, version, key, key_id=0):
    b = bytearray(blob)
    bf.parse(bytes(b))                      # the input must be a sound blob
    struct.pack_into("<I", b, 16, version)
    struct.pack_into("<I", b, 20, device_id)
    struct.pack_into("<I", b, 36, key_id)
    n = len(b)
    struct.pack_into("<I", b, n - bf.TRAILER_SIZE, zlib.crc32(bytes(b[:n - bf.TRAILER_SIZE])))
    b[n - 16:] = cmac(device_key(key, device_id), bytes(b[:n - 16]))
    return bytes(b)


def verify(blob, key, device_id):
    return cmac(device_key(key, device_id), bytes(blob[:-16])) == bytes(blob[-16:])


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("blob", nargs="?")
    ap.add_argument("--device-id", type=lambda s: int(s, 0))
    ap.add_argument("--version", type=lambda s: int(s, 0))
    ap.add_argument("--key", help="master key file: 16 raw bytes or 32 hex digits")
    ap.add_argument("--key-id", type=int, default=0)
    ap.add_argument("--out")
    ap.add_argument("--new-key", help="write a new random master key to this file and stop")
    args = ap.parse_args()

    if args.new_key:
        if os.path.exists(args.new_key):
            raise SystemExit("%s exists: not overwriting a key" % args.new_key)
        os.makedirs(os.path.dirname(os.path.abspath(args.new_key)), exist_ok=True)
        with open(args.new_key, "wb") as f:
            f.write(os.urandom(16))
        print("Wrote a new master key to %s. Back it up somewhere safe; it is not in git." % args.new_key)
        return

    if not (args.blob and args.key and args.out) or args.device_id is None or args.version is None:
        ap.error("need a blob, --device-id, --version, --key and --out")
    if args.device_id == bf.DEVICE_ANY:
        raise SystemExit("A signed blob must name one device, not the wildcard")

    with open(args.blob, "rb") as f:
        blob = f.read()
    key = read_key(args.key)
    signed = sign(blob, args.device_id, args.version, key, args.key_id)
    h = bf.parse(signed)
    if not verify(signed, key, args.device_id):
        raise SystemExit("Internal error: the signature does not verify")
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as f:
        f.write(signed)
    print("Signed for device %d, version %d: model %08x, %d bytes, CRC %08x, MAC %s"
          % (h["device_id"], h["version"], h["model_id"], len(signed), h["crc"], h["mac"].hex()))
    print("Wrote %s" % args.out)


if __name__ == "__main__":
    main()
