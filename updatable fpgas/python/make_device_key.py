"""Write one board's keys into a file the FPGA build reads.

    py -3.13 make_device_key.py --key keys/master.key --device-id 1

writes keys/device_key.vh (keys/ is ignored by git) with the board number, its
stamp key and its scrambling key, worked out from the master key as
blob_crypto.py says. rtl/top/updatable_top.v includes this file: without it the
Quartus build stops, so a board can never be built with a missing or test key.
One file per board: build each board's program file with its own.

    py -3.13 make_device_key.py --test

rewrites tb/test_key.vh from the public test key (simulation only; that one is
in git on purpose).

The board key ends up inside the program file and in Quartus' reports: do not
share the quartus/ output folder.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blob_crypto as bc  # noqa: E402
import blob_format as bf  # noqa: E402
import paths  # noqa: E402
from sign_blob import read_key  # noqa: E402


def text(prefix, device_id, master, note):
    return ("// %s\n"
            "// Made by python/make_device_key.py. Do not edit by hand.\n"
            "`define %sDEVICE_ID 32'h%08X\n"
            "`define %sMAC_KEY   %s\n"
            "`define %sENC_KEY   %s\n"
            % (note, prefix, device_id, prefix, bc.verilog_hex(bc.mac_key(master, device_id)),
               prefix, bc.verilog_hex(bc.enc_key(master, device_id))))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--key", help="master key file (sign_blob.py --new-key makes one)")
    ap.add_argument("--device-id", type=lambda s: int(s, 0))
    ap.add_argument("--out", default=os.path.join(paths.UPDATABLE, "keys", "device_key.vh"))
    ap.add_argument("--force", action="store_true", help="overwrite an existing key file")
    ap.add_argument("--test", action="store_true", help="write tb/test_key.vh from the public test key")
    args = ap.parse_args()

    if args.test:
        out = os.path.join(paths.UPDATABLE, "tb", "test_key.vh")
        with open(out, "w", newline="\n") as f:
            f.write(text("TEST_", 1, bc.TEST_MASTER_KEY,
                         "PUBLIC TEST KEYS for device 1, simulation only (blob_crypto.TEST_MASTER_KEY)."))
        print("Wrote %s" % out)
        return

    if not args.key or args.device_id is None:
        ap.error("need --key and --device-id (or --test)")
    if args.device_id == bf.DEVICE_ANY:
        raise SystemExit("A board needs its own device ID, not the wildcard FFFFFFFF")
    if os.path.exists(args.out) and not args.force:
        raise SystemExit("%s exists: not overwriting a key file (use --force to replace it)" % args.out)
    master = read_key(args.key)
    if master == bc.TEST_MASTER_KEY:
        raise SystemExit("That is the public test key: make a real one with sign_blob.py --new-key")
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", newline="\n") as f:
        f.write(text("BOARD_", args.device_id, master,
                     "SECRET: board keys for device %d. Never put this file in git." % args.device_id))
    print("Wrote %s for device %d. Rebuild the Quartus project to use it." % (args.out, args.device_id))


if __name__ == "__main__":
    main()
