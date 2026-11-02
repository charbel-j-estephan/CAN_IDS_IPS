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
    "run_check_esp32.bat": "run_check_esp32.bat",
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
    "run_check_stm32_IDS.bat": "run_check_stm32_IDS.bat",
    "run_check_stm32_IPS.bat": "run_check_stm32_IPS.bat",
    "docs/STM32_WIRING.md": "STM32_WIRING.md",
    "mcu/stm32_ids/platformio.ini": "stm32_ids/platformio.ini",
    "mcu/stm32_ids/boards/black_f407ve_usb.json": "stm32_ids/boards/black_f407ve_usb.json",
    "mcu/stm32_ids/src/main.cpp": "stm32_ids/src/main.cpp",
    "mcu/stm32_ids/src/can_rx.c": "stm32_ids/src/can_rx.c",
    "mcu/stm32_ids/src/can_rx.h": "stm32_ids/src/can_rx.h",
    "mcu/stm32_ids/src/classify.c": "stm32_ids/src/classify.c",
    "mcu/stm32_ids/src/classify.h": "stm32_ids/src/classify.h",
    "mcu/stm32_ids/src/can_ids_model.h": "stm32_ids/src/can_ids_model.h",
    "mcu/stm32_ids/src/can_ids_model_p6.h": "stm32_ids/src/can_ids_model_p6.h",
    "mcu/stm32_ids/src/can_ids_model_p8.h": "stm32_ids/src/can_ids_model_p8.h",
    "mcu/stm32_ids/test/host_test.c": "stm32_ids/test/host_test.c",
}

# Files and folders from earlier versions of the bench.
OBSOLETE = [
    "python/esp32_eval.py",
    "python/send_dataset.py",
    "python/__pycache__",
    "esp32/wifi_receiver",
    "stm32/gateway",
    "upload_stm32.bat",
    "python/stm32_dfu.py",
    "stm32",
    "run_check.bat",                  # renamed 11-02: run_check_esp32.bat,
    "run_check_stm32.bat",            # run_check_stm32_IDS.bat and
    "run_check_ips.bat",              # run_check_stm32_IPS.bat
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
