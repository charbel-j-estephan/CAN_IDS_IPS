"""Copy the CAN bench from the repo (the master copy) to the folder the runs are
done from, and remove files there that the bench no longer uses.

    python python/sync_bench.py
    python python/sync_bench.py --dest "D:\\some\\other\\folder"

Never touches the dataset (data/) or the results (runs/) in the destination.
"""
import argparse
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_DEST = os.path.join(os.path.expanduser("~"), "Desktop", "esp32 setup", "can_replay")

# repo path -> path in the destination
FILES = {
    "run_check.bat": "run_check.bat",
    "docs/ESP32_WIRING.md": "README.md",
    "python/replay_dataset.py": "python/replay_dataset.py",
    "python/report_html.py": "python/report_html.py",
    "python/can_bits.py": "python/can_bits.py",
    "python/serial_link.py": "python/serial_link.py",
    "python/run_check.py": "python/run_check.py",
    "esp32/replay_sender/replay_sender.ino": "esp32/replay_sender/replay_sender.ino",
    "esp32/usb_receiver/usb_receiver.ino": "esp32/usb_receiver/usb_receiver.ino",
    "esp32/attack_sender/attack_sender.ino": "esp32/attack_sender/attack_sender.ino",
    "esp32/link_sender/link_sender.ino": "esp32/link_sender/link_sender.ino",
    "esp32/link_receiver/link_receiver.ino": "esp32/link_receiver/link_receiver.ino",
}

# Files and folders from earlier versions of the bench.
OBSOLETE = [
    "python/esp32_eval.py",
    "python/send_dataset.py",
    "python/__pycache__",
    "esp32/wifi_receiver",
    "stm32/gateway",
    "stm32",
]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dest", default=DEFAULT_DEST)
    args = ap.parse_args()

    for src, dst in FILES.items():
        s = os.path.join(ROOT, src)
        d = os.path.join(args.dest, dst)
        os.makedirs(os.path.dirname(d), exist_ok=True)
        shutil.copyfile(s, d)
        print("copied   %s" % dst)

    for rel in OBSOLETE:
        p = os.path.join(args.dest, rel)
        if os.path.isdir(p):
            if rel == "stm32" and os.listdir(p):
                continue                    # only remove the folder once it is empty
            shutil.rmtree(p)
            print("removed  %s" % rel)
        elif os.path.isfile(p):
            os.remove(p)
            print("removed  %s" % rel)
    print("\n%s is up to date with the repo." % args.dest)


if __name__ == "__main__":
    main()
