"""Push a signed model blob to the FPGA through the STM32 update tool.

    py -3.13 push_model.py COM5 build/signed/p5_dev1_v2.bin     upload and COMMIT
    py -3.13 push_model.py COM5                                 show status, version, counters

The FPGA's maintenance switch (SW[1]) must be up for an upload.
"""
import argparse
import struct
import sys
import time

import serial


def ask(port, cmd):
    port.write((cmd + "\n").encode())
    print(port.readline().decode(errors="replace").strip())


def push(port, data):
    if len(data) < 24:
        raise SystemExit("too short to be a blob")
    crc = struct.unpack_from("<I", data, len(data) - 20)[0]
    port.write(("PUSH %d %08x\n" % (len(data), crc)).encode())
    line = port.readline().decode(errors="replace").strip()
    if not line.startswith("READY"):
        raise SystemExit("STM32 says: %s" % line)
    port.write(data)
    port.flush()
    t0 = time.time()
    ok = False
    while time.time() - t0 < 30:
        line = port.readline().decode(errors="replace").strip()
        if line:
            print(line)
            if line.startswith("OK"):
                ok = True
            if line.startswith("VERSION") or line.startswith("ERROR"):
                if line.startswith("ERROR"):
                    raise SystemExit(1)
                break
    if not ok:
        raise SystemExit("The update did not finish.")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port")
    ap.add_argument("blob", nargs="?")
    args = ap.parse_args()
    port = serial.Serial(args.port, 115200, timeout=5)
    time.sleep(0.5)
    port.reset_input_buffer()
    if args.blob:
        with open(args.blob, "rb") as f:
            push(port, f.read())
    ask(port, "STATUS")
    ask(port, "VERSION")
    ask(port, "COUNTERS")
    port.close()


if __name__ == "__main__":
    sys.exit(main())
