"""One command for the whole test: find the boards, check the link, replay the
dataset, open the results page.

    python run_check.py                                  20,000 DoS frames
    python run_check.py --file gear_dataset.csv --n 100000
    python run_check.py --dry-run                        only show what it would do
    python run_check.py --stm32                          check the frames the STM32 decodes

Steps:
  1. Finds the ESP32s and tells the sender (replay_sender), the receiver
     (usb_receiver) and the attacker (attack_sender, optional third board)
     apart by what they print, so you never have to guess COM ports. With an
     attacker, normal and attack frames come from two separate boards.
  2. Link check: 300 frames sent flat out. Every one must arrive unchanged,
     otherwise it stops and says what to check.
  3. Replays the dataset with its original timing (replay_dataset.py).
  4. Opens report.html.

With --stm32 the STM32 IDS node (mcu/stm32_ids) is the receiver whose frames
are checked. The ESP32 receiver must still be on the bus to acknowledge
frames, since the STM32 only listens.
"""
import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
ESP32_VIDS = {0x1A86, 0x10C4, 0x0403}      # CH340/CH9102, CP210x, FTDI USB-serial chips
STM32_VID = 0x0483                         # the STM32's own USB serial port


def find_ports(vids=ESP32_VIDS):
    import serial.tools.list_ports as lp
    return [p.device for p in lp.comports() if p.vid in vids]


def identify(port, listen=2.5):
    """Reset the board and listen. The receiver prints #STAT every second, the
    sender prints READY, the attacker READY ATTACKER.
    Returns 'sender', 'receiver', 'attacker', 'busy' or 'unknown'."""
    import serial
    try:
        ser = serial.Serial(port, 921600, timeout=0.1)
    except serial.SerialException as e:
        return "busy" if "denied" in str(e).lower() or "access" in str(e).lower() else "unknown"
    ser.setDTR(False)
    ser.setRTS(True)                      # pulse EN: restart the sketch
    time.sleep(0.1)
    ser.setRTS(False)
    seen, t0 = b"", time.time()
    while time.time() - t0 < listen:
        seen += ser.read(512)
        if b"#STAT" in seen:
            break
    ser.close()
    if b"#STAT" in seen:
        return "receiver"
    if b"READY ATTACKER" in seen:
        return "attacker"
    if b"READY" in seen:
        return "sender"
    return "unknown"


def run_replay(args_list):
    cmd = [sys.executable, os.path.join(HERE, "replay_dataset.py")] + args_list
    print("\n> " + " ".join(cmd[1:]) + "\n")
    return subprocess.call(cmd, cwd=HERE)


def newest_run():
    runs = sorted(glob.glob(os.path.join(ROOT, "runs", "2*")), key=os.path.getmtime)
    return runs[-1] if runs else None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--file", default="DoS_dataset.csv",
                    help="DoS_dataset.csv, Fuzzy_dataset.csv, gear_dataset.csv or RPM_dataset.csv")
    ap.add_argument("--n", type=int, default=20000, help="frames to replay, 0 for the whole file")
    ap.add_argument("--sender", help="skip detection: the sender's COM port")
    ap.add_argument("--receiver", help="skip detection: the receiver's COM port")
    ap.add_argument("--attacker", help="skip detection: the attacker's COM port")
    ap.add_argument("--stm32", nargs="?", const="auto", metavar="COM",
                    help="check the frames the STM32 IDS node decodes (its port is found "
                         "automatically, or give it); the ESP32 receiver only ACKs")
    ap.add_argument("--skip-check", action="store_true", help="skip the 300 frame link check")
    ap.add_argument("--dry-run", action="store_true", help="find the boards and print the plan only")
    args = ap.parse_args()

    # 1. Boards
    esp = find_ports()
    sender, receiver, attacker = args.sender, args.receiver, args.attacker
    stm32 = None
    if args.stm32:
        found = find_ports({STM32_VID})
        stm32 = args.stm32 if args.stm32 != "auto" else (found[0] if len(found) == 1 else None)
        if not stm32:
            sys.exit("STM32 USB port %s. Is it running mcu/stm32_ids (BOOT0 jumper back on GND, "
                     "then RST)? Or give it: --stm32 COMx"
                     % ("not found" if not found else "unclear: " + ", ".join(found)))
        receiver = stm32
    if not (sender and receiver):
        print("ESP32 USB ports found: %s" % (", ".join(esp) or "none"))
        roles = {}
        for port in esp:
            roles[port] = identify(port)
            print("  %s: %s" % (port, roles[port]))
        busy = [p for p, r in roles.items() if r == "busy"]
        if busy:
            sys.exit("\n%s is open in another program. Close the Arduino Serial Monitor "
                     "(or the whole Arduino IDE) and run this again." % ", ".join(busy))
        senders = [p for p, r in roles.items() if r == "sender"]
        receivers = [p for p, r in roles.items() if r == "receiver"]
        sender = sender or (senders[0] if len(senders) == 1 else None)
        receiver = receiver or (receivers[0] if len(receivers) == 1 else None)
        attackers = [p for p, r in roles.items() if r == "attacker"]
        attacker = attacker or (attackers[0] if len(attackers) == 1 else None)
        if len(attackers) > 1:
            sys.exit("\n%s all run attack_sender. Upload esp32/replay_sender to the board that "
                     "plays the normal car, then run this again." % ", ".join(attackers))
        if len(senders) > 1:
            sys.exit("\n%s all run replay_sender. Upload esp32/attack_sender to the attacker "
                     "board (or unplug it to run without an attacker)." % ", ".join(senders))
        if stm32 and sender and not receivers:
            print("  (no ESP32 receiver found on USB: it must still be powered and on the "
                  "bus to ACK frames)")
        if not sender or not receiver:
            sys.exit("\nCould not find exactly one sender and one receiver.\n"
                     "  - Both ESP32s must be plugged in with data cables.\n"
                     "  - The sender must run replay_sender, the receiver usb_receiver.\n"
                     "  - A board shown as 'unknown' printed nothing: re-upload its sketch.\n"
                     "Or name them yourself: --sender COMx --receiver COMy")
    print("\nSender:   %s" % sender)
    print("Receiver: %s%s" % (receiver, " (STM32 IDS node)" if stm32 else ""))
    print("Attacker: %s" % (attacker or "none, the sender plays the attacks too"))

    common = ["--sender", sender, "--receiver", receiver]
    full = common + ["--file", args.file, "--n", str(args.n)]
    if attacker:
        full += ["--attacker", attacker]
    if args.dry_run:
        print("\nWould run the link check, then: replay_dataset.py " + " ".join(full))
        return

    # 2. Link check
    if not args.skip_check:
        check_dir = os.path.join(ROOT, "runs", "_link_check")
        shutil.rmtree(check_dir, ignore_errors=True)
        print("\n=== Link check: 300 frames ===")
        code = run_replay(common + ["--file", args.file, "--n", "300", "--fast",
                                    "--out", check_dir])
        try:
            s = json.load(open(os.path.join(check_dir, "summary.json")))
        except (OSError, ValueError):
            s = None
        shutil.rmtree(check_dir, ignore_errors=True)
        if code != 0 or not s:
            sys.exit("\nThe link check did not finish. See the messages above.")
        if s["frames_arrived"] != s["frames_sent"] or s.get("unmatched_received"):
            sys.exit("\nLink check FAILED: %d of %d frames arrived, %d arrived altered.\n"
                     "Check, with the power off:\n"
                     "  - CANH to CANL reads about 60 ohm\n"
                     "  - CANH to CANH and CANL to CANL, not crossed\n"
                     "  - all grounds joined, every module powered"
                     % (s["frames_arrived"], s["frames_sent"], s.get("unmatched_received", 0)))
        print("\nLink check passed: %d of %d frames arrived unchanged." % (s["frames_arrived"], s["frames_sent"]))

    # 3. Replay
    print("\n=== Replay: %s, %s frames, original timing ===" % (args.file, args.n or "all"))
    before = newest_run()
    code = run_replay(full)
    after = newest_run()
    if code != 0 or after == before:
        sys.exit("\nThe replay did not finish. See the messages above.")

    # 4. Results page
    report = os.path.join(after, "report.html")
    print("\nOpening %s" % report)
    if hasattr(os, "startfile"):
        os.startfile(report)


if __name__ == "__main__":
    main()
