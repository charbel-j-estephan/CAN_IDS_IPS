"""Live demo, in three parts:

    1. the FPGA alone protects a real CAN bus (the STM32 only listens),
    2. the STM32 alone protects it, in software,
    3. the FPGA's model is updated over SPI, every step explained.

Run it with run_demo.bat (project folder):

    py -3.13 python/demo.py prepare   before the talk: STM32 -> IDS firmware, checks
    py -3.13 python/demo.py           the whole demo (about 15 minutes)
    py -3.13 python/demo.py attacks   parts 1 and 2 only
    py -3.13 python/demo.py update    part 3 only

Wiring and the checklist: docs/DEMO.md. The demo signs every model it pushes
with the master key (default Desktop\\master.key, or set CAN_MASTER_KEY), each
time with a version one above the board's, so it can be run again and again.
"""
import json
import os
import struct
import subprocess
import sys
import textwrap
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
ERRORS = {0: "ACCEPTED", 1: "REFUSED: maintenance mode is off (SW1 down)",
          2: "REFUSED: port locked", 6: "REFUSED: made for another board",
          7: "REFUSED: older version than the board's (rollback)", 10: "REFUSED: CRC",
          12: "REFUSED: stamp does not match (wrong key or edited file)", 16: "busy"}
ATTACKS = {"DoS": "DoS (flooding the bus with the highest priority ID)",
           "RPM": "RPM spoofing (a real engine ID with fake speed values)"}


# ---------------------------------------------------------------- screen

def banner(title):
    print("\n" + "=" * 72 + "\n  " + title + "\n" + "=" * 72)


def say(text, indent="  "):
    """A paragraph for the presenter to read or paraphrase."""
    for para in textwrap.dedent(text).strip().split("\n\n"):
        print(textwrap.fill(" ".join(para.split()), 72, initial_indent=indent,
                            subsequent_indent=indent))
        print()


def flush_keys():
    """Drop keys pressed ahead of a prompt, so one Enter too many does not
    skip the next step."""
    try:
        import msvcrt
        while msvcrt.kbhit():
            msvcrt.getwch()
    except ImportError:
        pass


def step(n, title, do, expect):
    print("\n" + "-" * 72)
    print("  STEP %s  %s" % (n, title))
    for line in do:
        print("  > " + line)
    print("  Expected: " + expect)
    print("-" * 72)
    flush_keys()
    input("  Press Enter when ready... ")


def wait_key(msg):
    flush_keys()
    input("  " + msg + " Press Enter... ")


def offer(url, what):
    print("\n  %s: %s" % (what, url))
    flush_keys()
    if input("  Open it now? [y/N] ").strip().lower() == "y":
        webbrowser.open(url)


# ---------------------------------------------------------------- STM32

def stm32_port():
    for p in list_ports.comports():
        if p.vid == 0x0483 and p.pid == 0x5740:
            return p.device
    return None


def stm32_runs():
    """'ids', 'updater' or None: the IDS firmware prints #STAT every second,
    the update tool answers ID."""
    port = stm32_port()
    if not port:
        return None
    with serial.Serial(port, 115200, timeout=0.3) as s:
        time.sleep(0.3)
        s.reset_input_buffer()
        s.write(b"ID\n")
        t0, seen = time.time(), b""
        while time.time() - t0 < 2.5:
            seen += s.read(512)
            if b"ID updater" in seen:
                return "updater"
            if b"#STAT" in seen and b"fw=" in seen:
                return "ids"
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
    """Shield off, BT0 to 3V3 + RST, flash the image, BT0 off 3V3, shield on, RST.
    Every jumper move waits for Enter: the shield covers the jumper."""
    print("\n  The STM32 gets new firmware: %s." % name)
    wait_key("On the STM32: take the shield off, put the BT0 jumper on 3V3, press RST.\n"
             "  When you are done changing it,")
    wait_for(dfu_present, "The STM32 in DFU mode")
    print("  Flashing %s..." % name)
    # Both IDs: the image's DFU suffix names the board's normal USB ID (0483:5740),
    # and dfu-util refuses the file if only the DFU-mode ID (0483:df11) is given.
    r = subprocess.run([DFU_UTIL, "-d", "0483:5740,0483:df11", "-a", "0", "-s", "0x08000000:leave",
                        "-D", image], capture_output=True, text=True)
    if "File downloaded successfully" not in (r.stdout + r.stderr):
        print(r.stdout[-800:], r.stderr[-800:])
        sys.exit("  Flashing failed. Press RST with BT0 on 3V3 and run this step again.")
    print("  Programmed.")
    wait_key("On the STM32: remove the BT0 jumper from 3V3 (BOOT0 back to GND),\n"
             "  put the shield back on, press RST. When you are done,")
    wait_for(lambda: stm32_port() is not None, "The STM32's USB port")
    time.sleep(1.5)
    wait_key("The STM32 runs %s. Ready to continue?" % name)


# ---------------------------------------------------------------- attacks

def replay(attack, detector):
    """One replay of 5,000 real frames; prints the result in plain words.
    detector: 'none', 'fpga' or 'stm32'."""
    files = {"DoS": "DoS_dataset.csv", "RPM": "RPM_dataset.csv"}
    cmd = [PY, os.path.join(ROOT, "python", "run_check.py"), "--file", files[attack],
           "--n", "5000"]
    # FPGA and no-protection runs: the STM32 records with its prevention
    # confirmed off, so the FPGA is alone on the bus.
    cmd += ["--ips"] if detector == "stm32" else ["--listen"]
    print("  Replaying 5,000 real car frames with their original timing (about 10 s)...")
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    out = r.stdout + r.stderr
    run_dir = None
    lines = out.splitlines()
    for i, line in enumerate(lines):
        if (line.startswith("Every frame is saved in") and i + 1 < len(lines)
                and "_link_check" not in lines[i + 1]):
            run_dir = lines[i + 1].strip()
    summ = os.path.join(run_dir, "summary.json") if run_dir else None
    if r.returncode or not summ or not os.path.exists(summ):
        print(out[-2500:])
        print("  The replay did not finish: see the messages above and docs/DEMO.md.")
        return None
    with open(os.path.join(run_dir, "demo_output.txt"), "w", encoding="utf-8") as f:
        f.write(out)
    s = json.load(open(summ))
    # The attack class is named after the file ('DoS', 'RPM_spoof', ...):
    # add up every class that is not Normal.
    atk = {"sent": 0, "arrived": 0, "lost": 0}
    for k, c in s["by_class"].items():
        if k != "Normal":
            for f in atk:
                atk[f] += c[f]
    nor = s["by_class"].get("Normal", {"sent": 0, "arrived": 0, "lost": 0})
    a = s.get("nodes", {}).get("attacker", {})
    who = {"none": "nobody", "fpga": "the FPGA", "stm32": "the STM32"}[detector]

    print("\n  RESULT  %s, protected by %s" % (ATTACKS[attack], who))
    print("  " + "." * 60)
    print("    Attack frames sent ................. %6s" % format(atk["sent"], ","))
    print("    ... reached the car's computers .... %6s" % format(atk["arrived"], ","))
    print("    ... destroyed on the wire .......... %6s" % format(atk["lost"], ","))
    print("    Normal car frames sent ............. %6s" % format(nor["sent"], ","))
    print("    ... lost ........................... %6s" % format(nor["lost"], ","))
    state = a.get("state", "?")
    print("    The attacker ....................... %s" % (
        "switched itself OFF the bus (bus off)" if state == "busoff" else
        "still sending (%s)" % state))
    print("  " + "." * 60)
    if atk["lost"] == 0:
        line = ("Every one of the %s attack frames reached the car. Nothing on a CAN bus "
                "can tell an attacker from a real computer." % format(atk["sent"], ","))
    elif atk["arrived"] == 0 and nor["lost"] == 0:
        line = ("All %s attack frames were destroyed before the end of the frame, so "
                "no computer in the car accepted one, and not one normal frame was lost. "
                "Each destroyed frame counts as an error against the attacker; after "
                "enough errors its own CAN chip took it off the bus."
                % format(atk["sent"], ","))
    else:
        line = ("%s of %s attack frames destroyed, %s normal frames lost."
                % (format(atk["lost"], ","), format(atk["sent"], ","),
                   format(nor["lost"], ",")))
    say("In one sentence: " + line)
    for x in s.get("lost_normal_check") or []:
        # The STM32 recorded the same bus: it tells whether a lost normal frame
        # was really on the wire (then the logging ESP32 missed it).
        print("  Lost normal frame, ID %s: %s" % (x["can_id"], x["why"]))
    print("  Results page: %s" % os.path.join(run_dir, "report.html"))
    return s


def need_ids():
    if stm32_runs() != "ids":
        say("""The STM32 must run the IDS firmware for the attack part, so it can
            listen (part 1) and protect (part 2).""")
        flash(FW_IDS, "the IDS firmware (fw 16)")


def part_fpga():
    banner("PART 1: the FPGA alone protects the bus")
    say("""On the bench: three ESP32 boards on a real CAN bus at 500 kbit/s. One
        replays normal traffic recorded in a real car, one is the attacker, one
        logs every frame that survives: that log is the car's point of view.
        The STM32 is on the bus too, but only listening in this part.""")
    say("""The FPGA (DE10-Lite) runs a random forest: 5 decision trees that look
        at the CAN ID, the length and the first 5 data bytes. It decides 300 ns
        after byte 5, while the rest of the frame is still on the wire. If it
        says attack, it sends an error flag: six bits that break the frame, so
        every computer on the bus throws it away. SW0 turns that on and off.""")
    need_ids()

    step("1", "No protection",
         ["DE10-Lite: SW0 DOWN (LEDR9 off): the FPGA only watches."],
         "every attack frame reaches the car.")
    replay("DoS", "none")

    step("2", "The FPGA protects the bus",
         ["DE10-Lite: SW0 UP (LEDR9 on)."],
         "every attack frame destroyed, 0 normal frames lost, attacker off the bus.")
    replay("DoS", "fpga")

    step("3", "A harder attack: spoofed engine speed",
         ["Keep SW0 UP.",
          "The attacker uses the engine's real ID and only fakes the data."],
         "every attack frame destroyed, 0 normal frames lost.")
    replay("RPM", "fpga")
    say("""Why this one is harder: the ID is a real one, so a list of allowed IDs
        cannot help. Only the trees, looking at the data bytes, can tell the
        fake speed values from the real ones.""")


def part_stm32():
    banner("PART 2: the STM32 alone protects the bus")
    say("""The same model, the same trees, now in C on a 168 MHz microcontroller.
        It has no CAN controller in use: it reads the bus bit by bit in
        software, runs the trees after byte 5 and sends the error flag itself.
        The FPGA is switched off from protecting (SW0 down).""")

    step("4", "The STM32 protects the bus",
         ["DE10-Lite: SW0 DOWN (the FPGA only watches)."],
         "every attack frame destroyed by the STM32, in software.")
    replay("DoS", "stm32")
    say("""The difference: the FPGA decides in 300 ns, the STM32 needs up to about
        90 microseconds, close to the time left before the frame ends. Both made
        it on this attack; the race between them is on the website.""")
    offer(SITE_COMPARE, "STM32 vs FPGA, who stops the attack in time")


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
            sys.exit("  The STM32 is not running the update tool (it said %r)." % ident)

    def drain(self):
        """Read and drop what is left of the last reply (a refused COMMIT prints
        STATUS and then VERSION), so the next command reads its own answer."""
        self.s.timeout = 0.3
        while self.s.readline():
            pass
        self.s.timeout = 5

    def ask(self, cmd):
        self.s.reset_input_buffer()
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
            sys.exit("  The FPGA does not answer (all FF): push both plugs of the SPI cable fully in;\n"
                     "  power the DE10-Lite before the STM32.")
        print("    model     %s" % MODELS.get(s["model"], "%08X" % s["model"]))
        print("    version   %d" % s["version"])
        print("    from      %s" % SOURCES.get(s["source"], s["source"]))
        print("    update    %s" % ("LOCKED" if s["locked"] else "port open" if s["maint"]
                                     else "port closed (SW1 down)"))
        if s["sig_fails"] is not None:
            print("    bad stamps since power on: %d (3 lock the port)" % s["sig_fails"])
        return s

    def need_sw1(self, up):
        """Wait until SW1 is where the step needs it: the board reports it."""
        while True:
            s = self.state()
            if s["raw"] or s["maint"] == up:
                return
            flush_keys()
            input("  SW1 is %s on the DE10-Lite. Put it %s, then press Enter... "
                  % (("UP", "DOWN") if not up else ("DOWN", "UP")))

    def push(self, blob):
        data = open(blob, "rb").read()
        crc = struct.unpack_from("<I", data, len(data) - 20)[0]
        self.drain()
        self.s.reset_input_buffer()
        self.s.write(("PUSH %d %08x\n" % (len(data), crc)).encode())
        line = self.s.readline().decode(errors="replace").strip()
        if not line.startswith("READY"):
            self.drain()
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
        self.drain()
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


def answer(u, blob):
    r = result(u.push(blob))
    print("\n  FPGA: %s" % ERRORS.get(r, "code %d" % r))
    return r


def hexs(b):
    return " ".join("%02X" % x for x in b)


def show_file(path):
    """The update file, field by field (docs: updatable fpgas/docs/BLOB_FORMAT.md)."""
    d = open(path, "rb").read()
    plain = open(MODEL, "rb").read()
    magic, fmt, flags, length, model_id, version, device = struct.unpack_from("<4sHHIIII", d, 0)
    print("    file           %s, %s bytes" % (os.path.basename(path), format(len(d), ",")))
    print("    header         %s, format %d, model %08X, version %d, for board %d"
          % (magic.decode(), fmt, model_id, version, device))
    print("    scrambled      %s, IV %s" % ("yes" if flags & 1 else "no", hexs(d[40:52])))
    print("    the trees      %s" % hexs(plain[704:720]))
    print("    as sent        %s" % hexs(d[704:720]))
    print("    CRC-32         %s" % hexs(d[-20:-16]))
    print("    stamp (CMAC)   %s" % hexs(d[-16:]))


def part_update():
    banner("PART 3: updating the FPGA's model, step by step")
    say("""What gets updated. The FPGA's circuit is fixed: a tree engine (8 trees x
        12 levels) programmed once with Quartus. The model it runs, the trees'
        questions, thresholds and answers, is data: 4,174 bytes kept in the
        FPGA's own flash, in two slots, A (LEDR4) and B (LEDR5). Updating the
        detector means sending a new model file, without Quartus, without a
        PC cable to the FPGA, while it keeps protecting the bus.""")
    say("""Who does what. The PC signs the file with a master key that never
        leaves the PC. The STM32 is only a courier: it receives the file over
        USB and sends it to the FPGA over SPI (4 wires: clock, data in, data
        out, select). It holds no key, so stealing it gives an attacker
        nothing. The FPGA holds its own key, inside its design, and decides.""")
    say("""The STM32 now changes job: from CAN detector to update tool.""")
    if stm32_runs() != "updater":
        flash(FW_UPDATER, "the update tool")
    offer(SITE_SIM, "The update, animated (step 5, Update)")
    u = Updater()

    step("5", "What the FPGA runs now",
         ["DE10-Lite: SW1 DOWN (maintenance off). SW0 stays where it is."],
         "the model, its version and the flash slot it booted from.")
    s = u.show()
    v = s["version"]
    # After program_fpga.bat the board runs its built-in model (version 0 or
    # 1); keep the rollback file strictly older than the new one.
    new_v = max(v + 1, 2)
    old_v = new_v - 1
    good = sign(new_v, os.path.join(WORK, "model_v%d.bin" % new_v))
    old = sign(old_v, os.path.join(WORK, "model_v%d.bin" % old_v))
    forged = sign(new_v + 1, os.path.join(WORK, "forged_v%d.bin" % (new_v + 1)), wrong_key())
    say("""Read from the FPGA over SPI. At power on it read its journal in flash,
        loaded the newest model, and checked that model's stamp before using it.
        Version %d is also the board's floor: it will never accept anything
        older.""" % v)

    step("6", "The update file, made and signed on the PC",
         ["Nothing to switch: this is what will travel to the FPGA."],
         "a header, the trees scrambled, a CRC and a 16-byte stamp.")
    show_file(good)
    print()
    say("""Header: which model, version %d, and for which board (board 1). A file
        made for another board is refused.""" % new_v)
    say("""Scrambled: the trees are encrypted with AES-128 in counter mode, with a
        new random IV for every file. Compare 'the trees' with 'as sent': the
        same bytes, unreadable on the wire. Someone who copies the file learns
        nothing about the model.""")
    say("""Stamp: an AES-CMAC over every byte before it, made with this board's
        key. The board's key is derived from the master key and the board
        number, so each board has its own. Change one bit anywhere in the file
        and the stamp no longer matches. Nobody without the key can make one.""")

    step("7", "Send it with the maintenance switch off",
         ["Keep SW1 DOWN (LEDR8 off).",
          "The STM32 sends this correctly signed version %d." % new_v],
         "refused: the update port only opens with the switch on the board.")
    u.need_sw1(0)
    answer(u, good)
    say("""Even a perfect file is refused: updates need someone at the board to flip
        a physical switch. Malware on the PC, or on the STM32, cannot update the
        FPGA on its own.""")

    step("8", "Send it with the maintenance switch on",
         ["Put SW1 UP (LEDR8 on).",
          "Watch LEDR4 / LEDR5: the new model goes to the slot that is NOT running."],
         "accepted: version %d, running from the other slot." % new_v)
    u.need_sw1(1)
    r = answer(u, good)
    u.show()
    print()
    if r == 0:
        say("""What the FPGA just did, in order (about a second):""")
        for n, t in enumerate([
                "received the 4,174 bytes over SPI into a spare memory bank, computing "
                "the stamp as the bytes came in",
                "checked the header: right format, right protocol (CAN), right board",
                "checked the version: %d is above its floor of %d" % (new_v, v),
                "checked the length and the CRC (nothing lost on the wire)",
                "checked the stamp against its own key: it matches",
                "unscrambled the trees, checked every tree pointer, and ran the test "
                "frames inside the file through the new trees: the answers match",
                "erased the free flash slot, wrote the model, read it back to compare",
                "wrote a journal record: newest slot and the new floor (a power cut at "
                "any moment leaves the old model or the new one, never half of one)",
                "swapped to the new model in a gap between two CAN frames: protection "
                "never stopped"], 1):
            print(textwrap.fill(t, 72, initial_indent="    %d. " % n,
                                subsequent_indent="       "))
        print()

    step("9", "Rollback: send the older version %d again" % old_v,
         ["Keep SW1 up.", "This file is correctly signed, but older than what the board has."],
         "refused: older version.")
    u.need_sw1(1)
    answer(u, old)
    say("""An attacker who kept an old, genuine model with a known weakness cannot
        bring it back: the floor is in flash and survives power cuts.""")

    step("10", "A forged model: signed with the attacker's own key",
         ["Keep SW1 up.", "Same model, version %d, but the stamp made with another key."
          % (new_v + 1)],
         "refused, model unchanged, 1 bad stamp counted.")
    u.need_sw1(1)
    answer(u, forged)
    u.show()
    print()
    say("""The stamp does not match, so nothing was written and the model did not
        change. Each bad stamp is counted: after three, the update port locks
        until the board is powered off and on, so nobody can try key after key.
        Only someone at the board can unlock it.""")

    step("11", "Close the port",
         ["Put SW1 DOWN (LEDR8 off)."],
         "port closed; the new model stays, even through a power cut.")
    u.need_sw1(0)
    u.show()
    u.close()
    say("""The model lives in the FPGA's flash: unplug the board, plug it back,
        and it starts version %d again, checking its stamp first. The STM32 can
        be unplugged; it took no part in the decision.""" % new_v)


# ---------------------------------------------------------------- top

def checks():
    missing = [p for p in (MASTER_KEY, MODEL, FW_UPDATER, FW_IDS, DFU_UTIL) if not os.path.exists(p)]
    if missing:
        sys.exit("  Missing:\n    " + "\n    ".join(missing) +
                 "\n  (the model blob comes from 'updatable fpgas\\run_sim.bat' step 1)")


def prepare():
    banner("PREPARE: before the talk")
    checks()
    print("  Wires: both shields on, the SPI cable between them, the bus connectors on.")
    print("  DE10-Lite: SW0 down, SW1 down. Power: the DE10-Lite first, then the STM32.")
    runs = stm32_runs()
    if runs is None:
        sys.exit("  The STM32 does not answer on USB. BT0 on GND, press RST, run this again.")
    if runs == "updater":
        u = Updater()
        print("\n  The FPGA holds:")
        u.show()
        u.close()
        flash(FW_IDS, "the IDS firmware (fw 16)")
    print("\n  Ready: the STM32 runs the IDS firmware. Leave SW0 and SW1 down;")
    print("  the demo asks for every switch.")


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    if mode == "prepare":
        prepare()
        return
    if mode not in ("all", "attacks", "update"):
        sys.exit(__doc__)
    checks()
    if mode in ("all", "attacks"):
        part_fpga()
        part_stm32()
    if mode in ("all", "update"):
        part_update()
    banner("Demo done. Every attack run is in runs\\ with its results page.")
    print("  To run it again: run_demo.bat prepare, then run_demo.bat")


if __name__ == "__main__":
    main()
