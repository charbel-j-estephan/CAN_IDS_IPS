"""Live demo: the secure model update on the FPGA, then the FPGA and the STM32
stopping attacks on a real CAN bus. Run it with run_demo.bat (project folder).

    py -3.13 python/demo.py prepare   before the talk: STM32 -> update tool, checks
    py -3.13 python/demo.py           the demo itself (about 10 minutes)
    py -3.13 python/demo.py attacks   only the attack part (STM32 already the IDS)

Wiring and the checklist: docs/DEMO.md. The demo signs every model it pushes
with the master key (default Desktop\\master.key, or set CAN_MASTER_KEY), each
time with a version one above the board's, so it can be run again and again.
"""
import os
import struct
import subprocess
import sys
import time
import webbrowser

import serial
from serial.tools import list_ports

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UPD = os.path.join(ROOT, "updatable fpgas")
PY = sys.executable
MASTER_KEY = os.environ.get("CAN_MASTER_KEY",
                            os.path.join(os.path.expanduser("~"), "Desktop", "master.key"))
WORK = os.path.join(UPD, "build", "demo")
MODEL = os.path.join(UPD, "build", "blobs", "p5", "model.bin")
FW_UPDATER = os.path.join(ROOT, "mcu", "prebuilt", "stm32_updater.bin")
FW_IDS = os.path.join(ROOT, "mcu", "prebuilt", "stm32_ids_fw16.bin")
DFU_UTIL = os.path.join(os.path.expanduser("~"), ".platformio", "packages",
                        "tool-dfuutil", "bin", "dfu-util.exe")
SITE_SIM = "https://charbel-j-estephan.com/CAN-IDS-IPS"
SITE_COMPARE = "https://charbel-j-estephan.com/CAN-COMPARE"

MODELS = {0x2DDDA531: "5-byte model (rf5-d9-p5)", 0xC0F7E8B5: "8-byte model (rf5-d9-p8)",
          0xFFFFFFFF: "no answer"}
SOURCES = {0: "none", 1: "flash slot A (LEDR4)", 2: "flash slot B (LEDR5)", 3: "boot ROM"}
ERRORS = {0: "OK", 1: "refused: maintenance mode is off (SW1 down)", 2: "refused: port locked",
          7: "refused: older version than the board's (rollback)", 10: "refused: CRC",
          12: "refused: stamp does not match (wrong key or edited file)", 16: "busy"}


# ---------------------------------------------------------------- screen

def banner(title):
    print("\n" + "=" * 72 + "\n  " + title + "\n" + "=" * 72)


def step(n, title, do, expect):
    print("\n" + "-" * 72)
    print("  STEP %s  %s" % (n, title))
    for line in do:
        print("  > " + line)
    print("  Expected: " + expect)
    print("-" * 72)
    input("  Press Enter when ready... ")


def wait_key(msg):
    input("  " + msg + " Press Enter... ")


# ---------------------------------------------------------------- STM32 ports

def stm32_port():
    for p in list_ports.comports():
        if p.vid == 0x0483 and p.pid == 0x5740:
            return p.device
    return None


def dfu_present():
    out = subprocess.run([DFU_UTIL, "-l"], capture_output=True, text=True).stdout
    return "0483:df11" in out.lower()


def wait_for(cond, what, timeout=60):
    t0 = time.time()
    while time.time() - t0 < timeout:
        if cond():
            return True
        time.sleep(0.5)
    sys.exit("\n  %s did not appear in %d s. Check the jumper and press RST." % (what, timeout))


def flash(image, name):
    """BT0 to 3V3 + RST, flash the image, BT0 to GND + RST."""
    print("\n  On the STM32: move the BT0 jumper to 3V3, then press RST.")
    wait_for(dfu_present, "The STM32 in DFU mode")
    print("  Flashing %s..." % name)
    r = subprocess.run([DFU_UTIL, "-d", "0x0483:0xDF11", "-a", "0", "-s", "0x08000000:leave",
                        "-D", image], capture_output=True, text=True)
    if "File downloaded successfully" not in (r.stdout + r.stderr):
        print(r.stdout[-800:], r.stderr[-800:])
        sys.exit("  Flashing failed. Press RST with BT0 on 3V3 and run this step again.")
    print("  Done. Now move BT0 back to GND and press RST.")
    wait_for(lambda: stm32_port() is not None, "The STM32's USB port")
    time.sleep(1.5)


# ---------------------------------------------------------------- update tool

class Updater:
    def __init__(self):
        port = stm32_port()
        if not port:
            sys.exit("  No STM32 on USB. Plug it in (BT0 on GND).")
        self.s = serial.Serial(port, 115200, timeout=5)
        time.sleep(0.5)
        self.s.reset_input_buffer()
        ident = self.ask("ID")
        if not ident.startswith("ID updater"):
            sys.exit("  The STM32 is not running the update tool (it said %r).\n"
                     "  Run: run_demo.bat prepare" % ident)

    def ask(self, cmd):
        self.s.write((cmd + "\n").encode())
        return self.s.readline().decode(errors="replace").strip()

    def state(self):
        st = self.ask("STATUS")
        ver = self.ask("VERSION")
        f = dict(kv.split("=") for kv in st.split()[2:])
        v = dict(kv.split("=") for kv in ver.split()[1:])
        s = {"err": int(f["err"]), "locked": int(f["locked"]), "maint": int(f["maint"]),
             "source": int(f["source"]), "model": int(v["model"], 16),
             "version": int(v["version"]), "sig_fails": None,
             "raw": st[:15] == "STATUS FFFFFFFF"}
        # COUNTERS only answers in maintenance mode (else it is refused and
        # counted as an update error), so ask only then.
        if s["maint"] and not s["raw"]:
            c = dict(kv.split("=") for kv in self.ask("COUNTERS").split()[1:])
            s["sig_fails"] = int(c["sig_fails"])
        return s

    def show(self, s=None):
        s = s or self.state()
        if s["raw"]:
            sys.exit("  The FPGA does not answer (all FF): check the five SPI wires, pin 8 and GND.")
        print("    model     %s" % MODELS.get(s["model"], "%08X" % s["model"]))
        print("    version   %d" % s["version"])
        print("    from      %s" % SOURCES.get(s["source"], s["source"]))
        print("    update    %s" % ("LOCKED" if s["locked"] else "port open" if s["maint"]
                                     else "port closed (SW1 down)"))
        if s["sig_fails"] is not None:
            print("    bad stamps since power on: %d (3 lock the port)" % s["sig_fails"])
        return s

    def push(self, blob):
        data = open(blob, "rb").read()
        crc = struct.unpack_from("<I", data, len(data) - 20)[0]
        self.s.write(("PUSH %d %08x\n" % (len(data), crc)).encode())
        line = self.s.readline().decode(errors="replace").strip()
        if not line.startswith("READY"):
            return "the update tool said: " + line
        self.s.write(data)
        self.s.flush()
        t0, last = time.time(), ""
        while time.time() - t0 < 30:
            line = self.s.readline().decode(errors="replace").strip()
            if line.startswith(("OK", "ERROR")):
                last = line
            if line.startswith("VERSION") or (line.startswith("ERROR") and "STATUS" in line):
                break
        return last

    def close(self):
        self.s.close()


def sign(version, out, key=None):
    os.makedirs(WORK, exist_ok=True)
    cmd = [PY, os.path.join(UPD, "python", "sign_blob.py"), MODEL, "--device-id", "1",
           "--version", str(version), "--key", key or MASTER_KEY, "--encrypt", "--out", out]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        sys.exit("  Signing failed:\n" + r.stdout + r.stderr)
    return out


def wrong_key():
    k = os.path.join(WORK, "attacker.key")
    if not os.path.exists(k):
        os.makedirs(WORK, exist_ok=True)
        subprocess.run([PY, os.path.join(UPD, "python", "sign_blob.py"), "--new-key", k],
                       capture_output=True, check=True)
    return k


def result(line):
    """'ERROR COMMIT failed, code 7: STATUS ...' -> the error code; 'OK ...' -> 0."""
    if line.startswith("OK"):
        return 0
    for kv in line.split():
        if kv.startswith("err="):
            return int(kv[4:])
    if "code" in line:
        return int(line.split("code")[1].split(":")[0])
    return -1


# ---------------------------------------------------------------- the parts

def checks():
    missing = [p for p in (MASTER_KEY, MODEL, FW_UPDATER, FW_IDS, DFU_UTIL) if not os.path.exists(p)]
    if missing:
        sys.exit("  Missing:\n    " + "\n    ".join(missing) +
                 "\n  (the model blob comes from 'updatable fpgas\\run_sim.bat' step 1)")


def prepare():
    banner("PREPARE: before the talk")
    checks()
    print("  The STM32 becomes the update tool, then the link to the FPGA is checked.")
    print("  Wires: the five SPI wires between the STM32 and the DE10-Lite (docs/DEMO.md).")
    print("  DE10-Lite: SW0 down, SW1 down.")
    port = stm32_port()
    ident = ""
    if port:
        with serial.Serial(port, 115200, timeout=2) as s:
            time.sleep(0.5)
            s.reset_input_buffer()
            s.write(b"ID\n")
            ident = s.readline().decode(errors="replace").strip()
    if not ident.startswith("ID updater"):
        flash(FW_UPDATER, "the update tool")
    u = Updater()
    print("\n  The FPGA now holds:")
    u.show()
    u.close()
    print("\n  Ready. Leave SW1 down; the demo asks for it.")


def part_update():
    banner("PART 1: a secure model update, live")
    print("  Background, on the big screen: %s (step 5, Update)" % SITE_SIM)
    if input("  Open it now? [y/N] ").strip().lower() == "y":
        webbrowser.open(SITE_SIM)
    u = Updater()

    step(1, "What the FPGA runs now",
         ["DE10-Lite: SW1 DOWN (maintenance off), SW0 down."],
         "the model, its version and the flash slot it booted from.")
    s = u.show()
    v = s["version"]
    new_v = v + 1
    good = sign(new_v, os.path.join(WORK, "model_v%d.bin" % new_v))
    old = sign(v - 1 if v > 1 else 1, os.path.join(WORK, "model_v%d.bin" % max(v - 1, 1)))
    forged = sign(new_v + 1, os.path.join(WORK, "forged_v%d.bin" % (new_v + 1)), wrong_key())

    step(2, "An update while maintenance mode is off",
         ["Keep SW1 DOWN.", "The STM32 sends a correctly signed version %d." % new_v],
         "refused: the update port only opens with the switch on the board.")
    r = result(u.push(good))
    print("  FPGA: %s" % ERRORS.get(r, "code %d" % r))

    step(3, "A real update: signed, scrambled, new version",
         ["Put SW1 UP (LEDR8 on).",
          "Watch LEDR4 / LEDR5: the new model goes to the OTHER flash slot."],
         "accepted: version %d, now running from the other slot." % new_v)
    r = result(u.push(good))
    print("  FPGA: %s" % ERRORS.get(r, "code %d" % r))
    u.show()

    step(4, "Rollback: send the older version %d again" % max(v - 1, 1),
         ["Keep SW1 up.", "This file is correctly signed, but older than what the board has."],
         "refused: an attacker cannot bring back an old model with a known weakness.")
    r = result(u.push(old))
    print("  FPGA: %s" % ERRORS.get(r, "code %d" % r))

    step(5, "A forged model: signed with the attacker's own key",
         ["Keep SW1 up.", "Same model, version %d, but the stamp was made with another key."
          % (new_v + 1)],
         "refused, model unchanged, 1 bad stamp counted (3 lock the port until power off).")
    r = result(u.push(forged))
    print("  FPGA: %s" % ERRORS.get(r, "code %d" % r))
    u.show()
    u.close()

    banner("PART 2: the STM32 becomes the CAN detector")
    print("  The update tool holds no keys and no model: unplug it, the FPGA keeps")
    print("  the new model in its own flash.")
    wait_key("Put SW1 DOWN. Unplug the five SPI wires (GND last).")
    flash(FW_IDS, "the IDS firmware (fw 16)")


def part_attacks():
    banner("PART 3: attacks on a real CAN bus")
    print("  The FPGA now runs the model pushed in part 1.")
    rc = [PY, os.path.join(ROOT, "python", "run_check.py")]

    step(6, "No protection",
         ["DE10-Lite: SW0 DOWN (LEDR9 off)."],
         "a DoS attack: every attack frame reaches the bus.")
    subprocess.run(rc + ["--file", "DoS_dataset.csv", "--n", "5000"])

    step(7, "The FPGA protects the bus",
         ["DE10-Lite: SW0 UP (LEDR9 on)."],
         "every DoS frame destroyed on the wire, 0 normal frames lost, attacker bus off.")
    subprocess.run(rc + ["--file", "DoS_dataset.csv", "--n", "5000", "--skip-check"])

    step(8, "A harder attack: spoofed engine speed (RPM)",
         ["Keep SW0 UP.", "A real car ID with fake data: only the trees can tell."],
         "all RPM frames destroyed, 0 normal frames lost.")
    subprocess.run(rc + ["--file", "RPM_dataset.csv", "--n", "5000", "--skip-check"])

    step(9, "The STM32 protects the bus instead",
         ["DE10-Lite: SW0 DOWN (the FPGA only listens)."],
         "every DoS frame destroyed by the STM32, in software.")
    subprocess.run(rc + ["--ips", "--file", "DoS_dataset.csv", "--n", "5000", "--skip-check"])

    print("\n  Who is faster, and why it matters: %s" % SITE_COMPARE)
    if input("  Open it now? [y/N] ").strip().lower() == "y":
        webbrowser.open(SITE_COMPARE)
    banner("Demo done. Every run is in runs\\ with its results page.")
    print("  To run the demo again: run_demo.bat prepare")


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    if mode == "prepare":
        prepare()
    elif mode == "attacks":
        part_attacks()
    elif mode == "all":
        checks()
        part_update()
        part_attacks()
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
