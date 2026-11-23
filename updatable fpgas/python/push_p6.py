"""Live field update for the paper: replace the FPGA's model with the 6 byte
model through the secure update path, with no Quartus and no new bitstream.

    py -3.13 push_p6.py            find the STM32 update tool and push
    py -3.13 push_p6.py COM5       give the port
    py -3.13 push_p6.py --dry-run  show what it would do

Steps it takes:
  1. Makes build/blobs/p6/model.bin from models/rf5_d9_p6_time.joblib if missing.
  2. Asks the FPGA (through the STM32 update tool) which model and version it runs.
  3. Signs and scrambles the 6 byte model for board 1 with version = that + 1,
     using Desktop\\master.key.
  4. Pushes it. SW1 (maintenance) must be up on the DE10-Lite.

Expected at the end: VERSION model=E7321E76 version=<N+1>, and LEDR4/LEDR5
switch to the other flash slot.

The STM32 must run the update tool (run_demo.bat update puts it on, or
updatable fpgas/docs/HOME_TEST_GUIDE.md part 3.1).
"""
import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
UPD = os.path.dirname(HERE)
ROOT = os.path.dirname(UPD)
PY = sys.executable
KEY = os.environ.get("CAN_MASTER_KEY", os.path.join(os.path.expanduser("~"), "Desktop", "master.key"))
MODEL = os.path.join(ROOT, "models", "rf5_d9_p6_time.joblib")
BLOB = os.path.join(UPD, "build", "blobs", "p6", "model.bin")
P6_ID = "E7321E76"


def run(cmd):
    print("> " + " ".join('"%s"' % c if " " in c else c for c in cmd))
    r = subprocess.run(cmd, cwd=UPD, capture_output=True, text=True)
    out = r.stdout + r.stderr
    print(out.rstrip())
    return r.returncode, out


def find_port():
    from serial.tools import list_ports
    found = [p.device for p in list_ports.comports() if p.vid == 0x0483]
    if len(found) != 1:
        sys.exit("STM32 port %s. Give it: py -3.13 push_p6.py COMx"
                 % ("not found" if not found else "unclear: " + ", ".join(found)))
    return found[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port", nargs="?")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    for p, what in ((KEY, "the master key"), (MODEL, "the 6 byte model")):
        if not os.path.exists(p):
            sys.exit("Missing %s: %s" % (what, p))
    port = args.port or find_port()

    if not os.path.exists(BLOB):
        code, _ = run([PY, os.path.join(HERE, "export_blob.py"), "--model", MODEL,
                       "--out", os.path.dirname(BLOB)])
        if code:
            sys.exit("export_blob.py failed")

    code, out = run([PY, os.path.join(HERE, "push_model.py"), port])
    m = re.search(r"VERSION model=([0-9A-Fa-f]{8}) version=(\d+)", out)
    if not m or m.group(1).upper() == "FFFFFFFF":
        sys.exit("The FPGA did not answer. Check the SPI cable and that the STM32 runs the update tool.")
    version = int(m.group(2)) + 1
    print("\nThe FPGA runs model %s version %s. The 6 byte model goes in as version %d."
          % (m.group(1).upper(), m.group(2), version))
    if " maint=1" not in out:
        print("NOTE: maintenance mode looks off. Put SW1 UP (LEDR8 on) before the push.")

    signed = os.path.join(UPD, "build", "signed", "p6_v%d.bin" % version)
    sign = [PY, os.path.join(HERE, "sign_blob.py"), BLOB, "--device-id", "1", "--version", str(version),
            "--key", KEY, "--encrypt", "--out", signed]
    push = [PY, os.path.join(HERE, "push_model.py"), port, signed]
    if args.dry_run:
        print("Would run:\n  %s\n  %s" % (" ".join(sign), " ".join(push)))
        return
    os.makedirs(os.path.dirname(signed), exist_ok=True)
    code, _ = run(sign)
    if code:
        sys.exit("sign_blob.py failed")
    code, out = run(push)
    if ("MODEL=" + P6_ID) in out.upper():
        print("\nPASS: the FPGA now runs the 6 byte model (%s), version %d." % (P6_ID, version))
    else:
        sys.exit("\nThe push did not end with model %s. Read the lines above (ERROR ... code N)." % P6_ID)


if __name__ == "__main__":
    main()
