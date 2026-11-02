"""One window for the whole project: see which boards are plugged in, upload the
STM32 firmware, build and program the FPGA, run the simulation and the bench
tests. Start it with run_GUI.bat (repo root).

Nothing here replaces the scripts; it runs them and shows their output:
  STM32 upload   platformio run -e black_f407ve_dfu -t upload  (mcu/stm32_ids)
  FPGA           run_sim_FPGA.bat, quartus_sh --flow compile, quartus_pgm
  bench tests    <bench>/python/run_check.py  (the copy in can_replay, which
                 finds the dataset and writes runs/ next to itself)

The status panel only lists USB devices; it never opens a port (opening an
ESP32's port restarts it, and a port held open blocks the test scripts).
"""
import glob
import os
import queue
import re
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk

import serial
import serial.tools.list_ports as list_ports

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
HOME = os.path.expanduser("~")
BENCH = os.path.join(HOME, "Desktop", "esp32 setup", "can_replay")
FIRMWARE = os.path.join(REPO, "mcu", "stm32_ids")
BOARD = os.path.join(REPO, "quartus_board")
PIO = os.path.join(HOME, ".platformio", "penv", "Scripts", "platformio.exe")
DFU = os.path.join(HOME, ".platformio", "packages", "tool-dfuutil", "bin", "dfu-util.exe")
QBIN = r"C:\altera_lite\25.1std\quartus\bin64"

STM32_VID, STM32_APP_PID = 0x0483, 0x5740     # stm32_ids running (USB serial)
STM32_DFU_ID = "0483:df11"                    # the STM32's ROM bootloader
ESP32_VIDS = {0x1A86, 0x10C4, 0x0403}         # CH340/CH9102, CP210x, FTDI
FILES = ["mixed", "DoS_dataset.csv", "Fuzzy_dataset.csv", "gear_dataset.csv", "RPM_dataset.csv"]
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
# The window runs under pythonw (no console); the scripts it starts get python.
PYEXE = sys.executable.replace("pythonw.exe", "python.exe")


# ---- what is plugged in ----------------------------------------------------------

def stm32_port():
    return next((p.device for p in list_ports.comports()
                 if p.vid == STM32_VID and p.pid == STM32_APP_PID), None)


def esp32_ports():
    return [p.device for p in list_ports.comports() if p.vid in ESP32_VIDS]


def stm32_in_bootloader():
    try:
        out = subprocess.run([DFU, "-l"], capture_output=True, text=True, timeout=8,
                             creationflags=NO_WINDOW).stdout
    except (OSError, subprocess.TimeoutExpired):
        return False
    return STM32_DFU_ID in out.lower()


def jtag_cable():
    """The first USB-Blaster quartus_pgm sees, e.g. 'USB-Blaster [USB-0]', or None."""
    try:
        out = subprocess.run([os.path.join(QBIN, "quartus_pgm.exe"), "-l"], capture_output=True,
                             text=True, timeout=30, creationflags=NO_WINDOW).stdout
    except (OSError, subprocess.TimeoutExpired):
        return None
    m = re.search(r"^\s*1\)\s*(.+?)\s*$", out, re.M)
    return m.group(1) if m else None


def board_bytes():
    """FEAT_BYTES in the board design (quartus_board/can_ids_top.v)."""
    try:
        m = re.search(r"localparam FEAT_BYTES = (\d+);", open(os.path.join(BOARD, "can_ids_top.v")).read())
        return int(m.group(1)) if m else None
    except OSError:
        return None


def set_board_bytes(n):
    """Point the board design at the n-byte model: FEAT_BYTES in can_ids_top.v and
    the generated_pN/rf_ids.v line in the .qsf, as the comments there describe."""
    top = os.path.join(BOARD, "can_ids_top.v")
    qsf = os.path.join(BOARD, "can_ids_top.qsf")
    s = open(top).read()
    open(top, "w").write(re.sub(r"localparam FEAT_BYTES = \d+;", "localparam FEAT_BYTES = %d;" % n, s))
    s = open(qsf).read()
    open(qsf, "w").write(re.sub(r"(VERILOG_FILE \.\./hdl/generated_p)\d(/rf_ids\.v)", r"\g<1>%d\g<2>" % n, s))


def read_fw(port, wait=4.0):
    """Open the STM32's port once and read its fw number from READY or #STAT."""
    try:
        with serial.Serial(port, 115200, timeout=0.2) as s:
            seen, t0 = b"", time.time()
            while time.time() - t0 < wait:
                seen += s.read(256)
                m = re.search(rb"fw=(\d+)", seen)
                if m:
                    return int(m.group(1))
    except serial.SerialException:
        return None
    return None


def prevention_off(port):
    """Send 'i' to the STM32 (prevention off). Used when an IPS run is stopped,
    since a killed script cannot do it itself."""
    try:
        with serial.Serial(port, 115200, timeout=0.2) as s:
            s.write(b"i")
            time.sleep(0.1)
        return True
    except serial.SerialException:
        return False


# ---- the window --------------------------------------------------------------------

class App:
    def __init__(self, root):
        self.root = root
        self.q = queue.Queue()            # (kind, value) from worker threads
        self.proc = None                  # the running command, if any
        self.job = None                   # its name, shown in the log
        self.cancel = threading.Event()
        self.busy = False
        self.ips_run = False
        root.title("CAN IDS/IPS bench")
        root.geometry("980x720")
        root.protocol("WM_DELETE_WINDOW", self.on_close)

        top = ttk.Frame(root, padding=8)
        top.pack(fill="x")
        self.status = {}
        st = ttk.LabelFrame(top, text="Plugged in", padding=6)
        st.pack(side="left", fill="y")
        for key, label in (("stm32", "STM32"), ("esp32", "ESP32s"), ("fpga", "FPGA cable")):
            row = ttk.Frame(st)
            row.pack(anchor="w", pady=1)
            ttk.Label(row, text=label + ":", width=11).pack(side="left")
            self.status[key] = ttk.Label(row, text="...", width=34)
            self.status[key].pack(side="left")

        # STM32
        f = ttk.LabelFrame(top, text="STM32 (5 data bytes)", padding=6)
        f.pack(side="left", fill="y", padx=8)
        self.buttons = []
        self.btn(f, "Upload firmware", self.stm32_upload)
        self.btn(f, "Read firmware version", self.stm32_version)

        # FPGA
        f = ttk.LabelFrame(top, text="FPGA", padding=6)
        f.pack(side="left", fill="y")
        row = ttk.Frame(f)
        row.pack(anchor="w")
        ttk.Label(row, text="Data bytes:").pack(side="left")
        self.fbytes = tk.IntVar(value=board_bytes() or 5)
        for n in (5, 6, 8):
            ttk.Radiobutton(row, text=str(n), variable=self.fbytes, value=n).pack(side="left")
        self.btn(f, "Simulate", self.fpga_sim)
        self.btn(f, "Compile board design", self.fpga_compile)
        self.btn(f, "Program the board", self.fpga_program)

        # Tests
        f = ttk.LabelFrame(root, text="Bench tests", padding=6)
        f.pack(fill="x", padx=8)
        self.test = tk.StringVar(value="STM32 IDS")
        ttk.Combobox(f, textvariable=self.test, state="readonly", width=12,
                     values=["ESP32 bench", "STM32 IDS", "STM32 IPS"]).pack(side="left")
        ttk.Label(f, text=" file:").pack(side="left")
        self.file = tk.StringVar(value="mixed")
        ttk.Combobox(f, textvariable=self.file, state="readonly", width=18, values=FILES).pack(side="left")
        ttk.Label(f, text=" frames:").pack(side="left")
        self.frames = tk.StringVar(value="20000")
        ttk.Entry(f, textvariable=self.frames, width=8).pack(side="left")
        self.btn(f, "Run test", self.run_test, side="left")
        ttk.Button(f, text="Open last report", command=self.open_report).pack(side="left", padx=4)

        # Step banner, stop, log
        bar = ttk.Frame(root, padding=(8, 6))
        bar.pack(fill="x")
        self.step = tk.Label(bar, text="", anchor="w", font=("Segoe UI", 11, "bold"), fg="#8a5a00")
        self.step.pack(side="left", fill="x", expand=True)
        self.stop_btn = ttk.Button(bar, text="Stop", command=self.stop, state="disabled")
        self.stop_btn.pack(side="right")
        ttk.Button(bar, text="Clear log", command=lambda: self.log.delete("1.0", "end")).pack(side="right", padx=4)
        self.log = tk.Text(root, wrap="none", font=("Consolas", 9))
        self.log.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        self.say("Bench folder: %s" % BENCH)
        if not os.path.isdir(BENCH):
            self.say("WARNING: the bench folder is missing; tests will not run (python python/sync_bench.py)")
        threading.Thread(target=self.poll, daemon=True).start()
        root.after(100, self.pump)

    def btn(self, parent, text, cmd, side=None):
        b = ttk.Button(parent, text=text, command=cmd)
        if side:
            b.pack(side=side, padx=4)
        else:
            b.pack(fill="x", pady=2)
        self.buttons.append(b)

    # -- plumbing between threads and the window --

    def say(self, text):
        self.log.insert("end", text.rstrip("\n") + "\n")
        self.log.see("end")

    def pump(self):
        try:
            while True:
                kind, val = self.q.get_nowait()
                if kind == "log":
                    self.say(val)
                elif kind == "step":
                    self.step.config(text=val)
                elif kind == "status":
                    key, text = val
                    self.status[key].config(text=text)
                elif kind == "done":
                    self.set_busy(False)
        except queue.Empty:
            pass
        self.root.after(100, self.pump)

    def set_busy(self, busy, job=None):
        self.busy = busy
        self.job = job if busy else None
        for b in self.buttons:
            b.config(state="disabled" if busy else "normal")
        self.stop_btn.config(state="normal" if busy else "disabled")
        if not busy:
            self.proc = None
            self.ips_run = False

    def start(self, job, work):
        """Run work() in a thread, one job at a time."""
        if self.busy:
            return
        self.cancel.clear()
        self.set_busy(True, job)
        self.q.put(("log", "\n===== %s =====" % job))

        def wrap():
            try:
                work()
            except Exception as e:                       # show it, never kill the window
                self.q.put(("log", "ERROR: %s" % e))
            finally:
                self.q.put(("step", ""))
                self.q.put(("done", None))
        threading.Thread(target=wrap, daemon=True).start()

    def run(self, cmd, cwd):
        """Run a command, stream its output to the log, return its exit code."""
        self.q.put(("log", "> " + " ".join('"%s"' % c if " " in c else c for c in cmd)))
        env = dict(os.environ, PYTHONUNBUFFERED="1")       # live output from scripts it starts too
        self.proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     stdin=subprocess.DEVNULL, text=True, errors="replace",
                                     creationflags=NO_WINDOW, env=env)
        for line in self.proc.stdout:
            self.q.put(("log", line))
        code = self.proc.wait()
        self.q.put(("log", "(exit code %d)" % code))
        return code

    def wait_for(self, check, text):
        """Show `text` and wait until check() is true, or Stop."""
        self.q.put(("step", text))
        while not self.cancel.is_set():
            if check():
                return True
            time.sleep(0.7)
        return False

    def stop(self):
        self.cancel.set()
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            self.q.put(("log", "Stopped."))
            if self.ips_run:
                threading.Thread(target=self.ips_cleanup, daemon=True).start()

    def ips_cleanup(self):
        time.sleep(1.0)                                  # let the killed script free the port
        port = stm32_port()
        ok = port and prevention_off(port)
        self.q.put(("log", "STM32 prevention switched off." if ok else
                    "Could not reach the STM32 to switch prevention off: press its RST button."))

    def on_close(self):
        if self.busy:
            self.stop()
            time.sleep(1.5)
        self.root.destroy()

    # -- status --

    def poll(self):
        last_jtag = 0
        while True:
            port = stm32_port()
            if port:
                text = "running firmware (%s)" % port
            elif not self.busy and stm32_in_bootloader():
                text = "BOOTLOADER (jumper on 3V3)"
            elif self.busy and self.job == "STM32 upload":
                text = "(uploading)"
            else:
                text = "not found"
            self.q.put(("status", ("stm32", text)))
            esp = esp32_ports()
            self.q.put(("status", ("esp32", "%d: %s" % (len(esp), ", ".join(esp)) if esp else "none")))
            if time.time() - last_jtag > 15 and not self.busy:
                cable = jtag_cable()
                self.q.put(("status", ("fpga", cable or "no USB-Blaster")))
                last_jtag = time.time()
            time.sleep(2)

    # -- STM32 --

    def stm32_upload(self):
        def work():
            if not stm32_in_bootloader():
                if not self.wait_for(stm32_in_bootloader,
                                     "Step 1/3: move the BOOT0 jumper to 3V3, then press RST "
                                     "(waiting for the bootloader...)"):
                    return
            self.q.put(("step", "Step 2/3: uploading..."))
            if self.run([PIO, "run", "-e", "black_f407ve_dfu", "-t", "upload"], FIRMWARE) != 0:
                self.q.put(("log", "Upload FAILED, see above. The jumper can stay on 3V3 to try again."))
                return
            if not self.wait_for(lambda: stm32_port() is not None,
                                 "Step 3/3: move the BOOT0 jumper back to GND, then press RST "
                                 "(waiting for the firmware...)"):
                return
            time.sleep(1.0)
            fw = read_fw(stm32_port())
            self.q.put(("log", "STM32 is running firmware fw=%s." % fw if fw else
                        "STM32 is back, but did not report its fw number."))
        self.start("STM32 upload", work)

    def stm32_version(self):
        def work():
            port = stm32_port()
            if not port:
                self.q.put(("log", "No STM32 running firmware is plugged in."))
                return
            fw = read_fw(port)
            self.q.put(("log", "STM32 on %s: fw=%s" % (port, fw) if fw else
                        "STM32 on %s did not answer (is a test script using it?)" % port))
        self.start("STM32 version", work)

    # -- FPGA --

    def fpga_sim(self):
        n = self.fbytes.get()
        self.start("FPGA simulation, %d bytes" % n,
                   lambda: self.run(["cmd", "/c", os.path.join(REPO, "run_sim_FPGA.bat"), str(n)], REPO))

    def fpga_compile(self):
        n = self.fbytes.get()

        def work():
            if board_bytes() != n:
                set_board_bytes(n)
                self.q.put(("log", "Board design set to the %d-byte model (FEAT_BYTES in can_ids_top.v and "
                            "the generated_p%d line in can_ids_top.qsf; both show in git diff)." % (n, n)))
            self.q.put(("step", "Compiling in Quartus (several minutes)..."))
            self.run([os.path.join(QBIN, "quartus_sh.exe"), "--flow", "compile", "can_ids_top"], BOARD)
        self.start("FPGA compile, %d bytes" % n, work)

    def fpga_program(self):
        def work():
            sofs = sorted(glob.glob(os.path.join(BOARD, "**", "*.sof"), recursive=True), key=os.path.getmtime)
            if not sofs:
                self.q.put(("log", "No .sof file yet: press 'Compile board design' first."))
                return
            cable = jtag_cable()
            if not cable:
                self.q.put(("log", "No USB-Blaster found. Plug in the DE10-Lite (USB) and try again."))
                return
            self.q.put(("log", "Programming %s (built with FEAT_BYTES=%s). The board forgets it at power "
                        "off." % (os.path.relpath(sofs[-1], REPO), board_bytes())))
            self.run([os.path.join(QBIN, "quartus_pgm.exe"), "-c", cable, "-m", "JTAG",
                      "-o", "p;%s" % sofs[-1]], BOARD)
        self.start("FPGA program", work)

    # -- bench tests --

    def run_test(self):
        test, f, n = self.test.get(), self.file.get(), self.frames.get().strip() or "20000"
        if not n.isdigit():
            self.say("Frames must be a number.")
            return
        script = os.path.join(BENCH, "python", "run_check.py")
        cmd = [PYEXE, "-u", script, "--file", f, "--n", n]
        if test == "STM32 IDS":
            cmd.append("--stm32")
        elif test == "STM32 IPS":
            cmd.append("--ips")
        elif f == "mixed":
            self.say("The ESP32 bench test plays one file; choose DoS, Fuzzy, gear or RPM.")
            return

        def work():
            self.ips_run = test == "STM32 IPS"
            self.q.put(("step", "%s test running (Stop ends it)..." % test))
            self.run(cmd, os.path.join(BENCH, "python"))
        self.start("%s test, %s, %s frames" % (test, f, n), work)

    def open_report(self):
        runs = sorted(glob.glob(os.path.join(BENCH, "runs", "2*")), key=os.path.getmtime)
        report = os.path.join(runs[-1], "report.html") if runs else None
        if report and os.path.exists(report):
            self.say("Opening %s" % report)
            os.startfile(report)
        else:
            self.say("No results page yet.")


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
