"""Upload mcu/stm32_ids to the STM32 without touching the BOOT0 jumper.

    python stm32_dfu.py              restart the STM32 into its USB bootloader, upload, restart
    python stm32_dfu.py --no-upload  only restart it into the bootloader

The running firmware (fw 3 and later) restarts into the STM32's built-in USB
bootloader when it receives "DFU!". Older firmware, or a board that does not
answer: use the jumper once (BT0 to 3V3, RST, upload, BT0 back to GND, RST).
"""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
STM32_VID, STM32_PID = 0x0483, 0x5740
PIO = os.path.join(os.path.expanduser("~"), ".platformio", "penv", "Scripts", "platformio.exe")
DFU_UTIL = os.path.join(os.path.expanduser("~"), ".platformio", "packages", "tool-dfuutil",
                        "bin", "dfu-util.exe")


def project_dir():
    """mcu/stm32_ids in the repo, stm32_ids in the bench folder."""
    for d in (os.path.join(ROOT, "mcu", "stm32_ids"), os.path.join(ROOT, "stm32_ids")):
        if os.path.isfile(os.path.join(d, "platformio.ini")):
            return d
    sys.exit("stm32_ids project not found next to this script")


def dfu_present():
    if not os.path.isfile(DFU_UTIL):
        return False
    out = subprocess.run([DFU_UTIL, "-l"], capture_output=True, text=True).stdout
    return "0483:df11" in out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="the STM32's COM port (found automatically)")
    ap.add_argument("--no-upload", action="store_true")
    args = ap.parse_args()

    if not dfu_present():
        import serial
        import serial.tools.list_ports as lp
        port = args.port or next((p.device for p in lp.comports()
                                  if p.vid == STM32_VID and p.pid == STM32_PID), None)
        if not port:
            sys.exit("No STM32 USB serial port and no bootloader found. Is the board plugged in?\n"
                     "Fallback: BT0 jumper to 3V3, press RST, run this again.")
        print("Asking the STM32 on %s to restart into its bootloader" % port)
        with serial.Serial(port, 921600, timeout=0.2) as s:
            s.write(b"DFU!")
            s.flush()
            time.sleep(0.3)
        t0 = time.time()
        while not dfu_present():
            if time.time() - t0 > 10:
                sys.exit("The bootloader did not appear. The firmware may be older than fw 3.\n"
                         "Fallback: BT0 jumper to 3V3, press RST, run this again.")
            time.sleep(0.5)
    print("STM32 bootloader (0483:df11) is there")
    if args.no_upload:
        return

    code = subprocess.call([PIO, "run", "-t", "upload", "-d", project_dir()])
    if code:
        sys.exit("Upload failed (see above). Unplug the STM32, plug it in again and retry.")
    print("\nUploaded. If the STM32 does not show up as a COM port within a few seconds, "
          "press RST (BT0 must be on GND).")


if __name__ == "__main__":
    main()
