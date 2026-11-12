"""Sign a model blob for one board, and scramble it if asked. Runs on your PC
only; the STM32 only ever carries the result.

    py -3.13 sign_blob.py build/blobs/p5/model.bin --device-id 1 --version 2 \\
        --key keys/master.key --encrypt --out build/signed/p5_dev1_v2.bin

    py -3.13 sign_blob.py --new-key keys/master.key     makes a random master key

Writes the device ID and version into the blob, scrambles the self tests, key
table and nodes with --encrypt (fresh random IV every time), recomputes the
CRC and fills the MAC field. Keys (blob_crypto.py):
    stamp key       = AES-CMAC(master key, device ID)
    scrambling key  = AES-CMAC(master key, "ENC" + device ID)
    MAC             = AES-CMAC(stamp key, every byte of the blob before the MAC)
A blob signed for one board fails on another. Keep keys/ out of git (it is
ignored) and off the STM32. The FPGA checks the MAC at every COMMIT and boot.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blob_crypto as bc  # noqa: E402
import blob_format as bf  # noqa: E402

# Kept for scripts that imported them from here.
cmac = bc.cmac


def device_key(master, device_id):
    return bc.mac_key(master, device_id)


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


def sign(blob, device_id, version, key, key_id=0, encrypt=False):
    h = bf.parse(bytes(blob))               # the input must be a sound blob
    if h["encrypted"]:
        raise SystemExit("The input is already scrambled: sign the plain blob from export_blob.py")
    b = bytearray(blob)
    struct.pack_into("<I", b, 16, version)
    struct.pack_into("<I", b, 20, device_id)
    struct.pack_into("<I", b, 36, key_id)
    if encrypt:                             # flag and IV are set before the CRC and the MAC
        b = bytearray(bc.scramble(bytes(b), key, device_id))
    return bc.seal(bytes(b), key, device_id)


def verify(blob, key, device_id):
    return bc.verify(blob, key, device_id)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("blob", nargs="?")
    ap.add_argument("--device-id", type=lambda s: int(s, 0))
    ap.add_argument("--version", type=lambda s: int(s, 0))
    ap.add_argument("--key", help="master key file: 16 raw bytes or 32 hex digits")
    ap.add_argument("--key-id", type=int, default=0)
    ap.add_argument("--encrypt", action="store_true",
                    help="scramble self tests, key table and nodes (AES-CTR, random IV)")
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
    signed = sign(blob, args.device_id, args.version, key, args.key_id, args.encrypt)
    h = bf.parse(signed)
    if not verify(signed, key, args.device_id):
        raise SystemExit("Internal error: the signature does not verify")
    if args.encrypt:
        plain = bf.parse(bc.unscramble(signed, key, args.device_id))
        if plain["model_id"] != h["model_id"] or not plain["trees"]:
            raise SystemExit("Internal error: the scrambled blob does not unscramble")
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as f:
        f.write(signed)
    print("Signed for device %d, version %d%s: model %08x, %d bytes, CRC %08x, MAC %s"
          % (h["device_id"], h["version"], ", scrambled" if args.encrypt else "",
             h["model_id"], len(signed), h["crc"], h["mac"].hex()))
    print("Wrote %s" % args.out)


if __name__ == "__main__":
    main()
