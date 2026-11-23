"""False flag test: what the STM32 IPS does to a REAL sender when its model
wrongly calls a genuine frame an attack.

The frames are a 20,000 frame stretch of normal_run_data.txt from the held out
part of the time split (data/fp_test/gear_dataset.csv, every frame marked R).
The 5 byte model flags 699 of them (669 on ID 0x43F, 30 on 0x0A0), see
data/fp_test/README.json. There is no attacker: every flag hits the sender.

What to look at afterwards in runs/<time>_gear_dataset/summary.json:
  - by_class.Normal.lost        frames the receiver never got
  - nodes.sender.state / tec    did the sender go bus off (the real ECU, in a car)
  - the IPS counters (rej=...)  how many flags the STM32 sent

run_check.py refuses an IPS run without an attacker on purpose, so this script
finds the boards itself and calls replay_dataset.py directly.

    python fp_test.py                 STM32 as the IPS, find the boards
    python fp_test.py --fpga          the FPGA is the IPS (SW0 up), no STM32 needed
    python fp_test.py --dry-run       print the command only

Expected flags on these frames: 5 byte model 699 (majority vote), 6 byte model
49, either model in strict mode (FPGA SW2 up) 0. Every flag on a genuine frame
is retransmitted and flagged again, so the sender is likely to end bus off.
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from run_check import ESP32_VIDS, STM32_VID, find_ports, identify  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender")
    ap.add_argument("--receiver")
    ap.add_argument("--ips", help="STM32 COM port (found automatically)")
    ap.add_argument("--fpga", action="store_true",
                    help="the FPGA is the IPS: no STM32 port, the FPGA acts on its own (SW0 up)")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    data = os.path.join(ROOT, "data", "fp_test")
    if not os.path.exists(os.path.join(data, "gear_dataset.csv")):
        sys.exit("data/fp_test/gear_dataset.csv is missing")

    sender, receiver, ips = args.sender, args.receiver, args.ips
    if not ips and not args.fpga:
        found = find_ports({STM32_VID})
        if len(found) != 1:
            sys.exit("STM32 port %s. Give it with --ips COMx" % ("not found" if not found else "unclear"))
        ips = found[0]
    if not (sender and receiver):
        roles = {p: identify(p) for p in find_ports(ESP32_VIDS)}
        for p, r in roles.items():
            print("  %s: %s" % (p, r))
        s = [p for p, r in roles.items() if r == "sender"]
        r = [p for p, r in roles.items() if r == "receiver"]
        if len(s) != 1 or len(r) != 1:
            sys.exit("Need exactly one sender and one receiver. Give them with --sender/--receiver")
        sender, receiver = sender or s[0], receiver or r[0]
    print("Sender %s, receiver %s, IPS %s. The attacker board is not used "
          "(it can stay plugged in, it sends nothing)."
          % (sender, receiver, "the FPGA (SW0 must be up)" if args.fpga else "STM32 on " + ips))

    cmd = [sys.executable, os.path.join(HERE, "replay_dataset.py"),
           "--sender", sender, "--receiver", receiver,
           "--data", data, "--file", "gear_dataset.csv", "--n", "0"]
    if not args.fpga:
        cmd += ["--ips", ips]
    print("> " + " ".join(cmd[1:]))
    if args.dry_run:
        return
    sys.exit(subprocess.call(cmd, cwd=HERE))


if __name__ == "__main__":
    main()
