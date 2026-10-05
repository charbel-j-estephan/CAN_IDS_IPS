"""Replay a Car Hacking dataset file onto a real CAN bus with its original timing,
record every frame the receiver hears, and write a results page.

    PC --USB--> ESP32 A (replay_sender) --CAN bus--> ESP32 B (usb_receiver) --USB--> PC
                                                     or (wifi_receiver) --WiFi--> PC

Every frame arrives here with the microsecond time ESP32 B received it. USB is
the reliable choice; WiFi frees a cable but a busy hotspot loses data.

    python python/replay_dataset.py --sender COM5 --receiver COM6     # receiver on USB
    python python/replay_dataset.py --sender COM5                     # receiver on WiFi
    python python/replay_dataset.py --sender COM5 --file Fuzzy_dataset.csv --n 100000
    python python/replay_dataset.py --sender COM5 --fast          # ignore timing, send flat out

Everything is saved under runs/<time>_<file>/ : frames.csv holds EVERY frame
sent (with whether it arrived and when), received.csv every frame heard,
summary.json the numbers, report.html the results page.

Needs numpy and pyserial (pip install numpy pyserial).
"""
import argparse
import csv
import json
import os
import socket
import sys
import threading
import time

import numpy as np

import esp32_eval as E
import report_html
from can_bits import frame_us

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLASS_NAMES = ["Normal", "DoS", "Fuzzy", "Gear_spoof", "RPM_spoof"]
FILE_CLASS = {"DoS_dataset.csv": 1, "Fuzzy_dataset.csv": 2,
              "gear_dataset.csv": 3, "RPM_dataset.csv": 4}
SYNC_TS = 0xA7
PACKET = 15


# ---- dataset with timestamps ----------------------------------------------------

def load_timed(path, n):
    """Rows of (timestamp s, can_id, dlc, d0..d7, class) from one attack CSV."""
    cls = FILE_CLASS[os.path.basename(path)]
    rows = []
    with open(path) as f:
        for line in f:
            p = line.strip().split(",")
            if len(p) < 4:
                continue
            try:
                ts, cid, dlc = float(p[0]), int(p[1], 16), int(p[2])
                data = [int(b, 16) for b in p[3:3 + dlc]]
            except ValueError:
                continue
            flag = p[3 + dlc].strip() if len(p) > 3 + dlc else "R"
            rows.append([ts, cid & 0x7FF, min(dlc, 8)] + data[:8] + [0] * (8 - len(data[:8]))
                        + [cls if flag == "T" else 0])
            if n and len(rows) >= n:
                break
    return rows


def pack_timed(row, rel_us):
    """15 byte packet for ESP32 A: id, dlc, 8 data bytes, microsecond offset."""
    cid, dlc = row[1], row[2]
    return bytes([cid >> 8, cid & 0xFF, dlc] + [int(b) for b in row[3:11]]) + \
        int(rel_us).to_bytes(4, "little")


def start_offsets(rows, realtime):
    """When ESP32 A must START each frame, in microseconds after the first one.

    The dataset stamps a frame when it ENDS (checked against the capture: the
    gap before a frame is never shorter than that frame's own length). So each
    frame starts one frame length earlier, and that length depends on its
    data through bit stuffing. 0 means "send now"."""
    if not realtime:
        return [0] * len(rows)
    starts = [r[0] * 1e6 - frame_us(r[1], r[2], r[3:3 + r[2]]) for r in rows]
    return [max(0, round(x - starts[0])) for x in starts]


# ---- receiving over WiFi --------------------------------------------------------

def parse_timed_stream(buf):
    """Records are 0xA7 id_hi id_lo dlc d0..d7 t0..t3 (16 bytes), lines start with '#'.

    Returns ([(can_id, dlc, d0..d7)], [timestamp us], [text lines])."""
    frames, stamps, texts = [], [], []
    i, n = 0, len(buf)
    while i < n:
        b = buf[i]
        if b == SYNC_TS:
            if i + 16 > n:
                break
            hi, lo, dlc = buf[i + 1], buf[i + 2], buf[i + 3]
            if hi < 8 and dlc <= 8:
                frames.append((hi << 8 | lo, dlc) + tuple(buf[i + 4:i + 12]))
                stamps.append(int.from_bytes(buf[i + 12:i + 16], "little"))
                i += 16
                continue
            i += 1
        elif b == 0x23:
            j = buf.find(b"\n", i)
            if j < 0:
                break
            texts.append(bytes(buf[i:j]).decode("ascii", "replace"))
            i = j + 1
        else:
            i += 1
    del buf[:i]
    return frames, stamps, texts


class Receiver(threading.Thread):
    """TCP server: ESP32 B connects to it and every frame is stored here.

    Frames are only kept once arm() is called. Anything earlier is frames the
    board buffered before this run (an earlier attempt, bus noise) and would
    otherwise be mistaken for part of this replay. If the WiFi link drops, the
    board reconnects and this accepts it again.
    """

    def __init__(self, port):
        super().__init__(daemon=True)
        self.srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.srv.bind(("0.0.0.0", port))
        self.srv.listen(1)
        self.srv.settimeout(0.5)
        self.frames, self.stamps, self.texts = [], [], []
        self.connected = threading.Event()
        self.armed = False
        self.connections = 0
        self.ignored = 0                   # frames received before arm()
        self.last_stat_time = 0.0          # the board sends #STAT once a second while it is alive
        self.last_data = time.time()       # time of the last FRAME received
        self.stop = False

    def arm(self):
        self.armed = True
        self.last_data = time.time()

    def run(self):
        while not self.stop:
            try:
                conn, addr = self.srv.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            self.peer = addr[0]
            self.connections += 1
            if self.connections > 1:
                print("  ESP32 B reconnected (link dropped once)")
            self.connected.set()
            self._read(conn)

    def _read(self, conn):
        buf = bytearray()                  # fresh per connection: a cut record is not glued to new data
        conn.settimeout(0.2)
        while not self.stop:
            try:
                data = conn.recv(65536)
            except socket.timeout:
                continue
            except OSError:
                break
            if not data:
                break
            buf += data
            f, s, t = parse_timed_stream(buf)
            self.texts += t
            if any(x.startswith("#STAT") for x in t):
                self.last_stat_time = time.time()
            if not f:                      # the once a second #STAT lines do not count
                continue
            self.last_data = time.time()
            if self.armed:
                self.frames += f
                self.stamps += s
            else:
                self.ignored += len(f)
        conn.close()

    def last_stat(self):
        return next((t for t in reversed(self.texts) if t.startswith("#STAT")), None)


class SerialReceiver(threading.Thread):
    """ESP32 B on a USB cable (usb_receiver sketch). Same interface as Receiver."""

    def __init__(self, ser):
        super().__init__(daemon=True)
        self.ser = ser
        self.peer = ser.port
        self.frames, self.stamps, self.texts = [], [], []
        self.connected = threading.Event()
        self.connected.set()
        self.armed = False
        self.connections = 1
        self.ignored = 0
        self.last_data = time.time()
        self.last_stat_time = 0.0
        self.stop = False

    arm = Receiver.arm
    last_stat = Receiver.last_stat

    def run(self):
        buf = bytearray()
        while not self.stop:
            data = self.ser.read(max(1, self.ser.in_waiting))
            if not data:
                continue
            buf += data
            f, s, t = parse_timed_stream(buf)
            self.texts += t
            if any(x.startswith("#STAT") for x in t):
                self.last_stat_time = time.time()
            if not f:
                continue
            self.last_data = time.time()
            if self.armed:
                self.frames += f
                self.stamps += s
            else:
                self.ignored += len(f)


def local_ips():
    ips = set()
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            ips.add(info[4][0])
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ips.add(s.getsockname()[0])
        s.close()
    except OSError:
        pass
    return sorted(i for i in ips if not i.startswith("127."))


# ---- analysis -------------------------------------------------------------------

def unwrap(stamps):
    """The ESP32 sends the low 32 bits of a microsecond clock, so undo the wrap."""
    out, base, prev = [], 0, None
    for s in stamps:
        if prev is not None and s < prev - (1 << 31):
            base += 1 << 32
        out.append(s + base)
        prev = s
    return out


MATCH_WINDOW_US = 20000      # how far a frame may be from its expected time
SEQ_WINDOW = 4096            # fast mode: how far ahead to look for a frame


def dedupe(frames, stamps):
    """Drop records the board sent twice (resent after a WiFi reconnect).

    A resent record carries the same board timestamp as the original, and two
    different frames can never share one microsecond on a 500 kbit/s bus."""
    seen, f2, s2 = set(), [], []
    for f, t in zip(frames, stamps):
        key = (t, f[0])
        if key in seen:
            continue
        seen.add(key)
        f2.append(f)
        s2.append(t)
    return f2, s2, len(frames) - len(f2)


def match_sequential(sent, frames):
    """Fast mode: frames keep their order, match each to the next identical one."""
    rx_for, spurious, p = [-1] * len(sent), 0, 0
    for k, g in enumerate(frames):
        end = min(len(sent), p + SEQ_WINDOW)
        j = p
        while j < end and sent[j] != g:
            j += 1
        if j < end:
            rx_for[j] = k
            p = j + 1
        else:
            spurious += 1
    return rx_for, spurious


def match_timed(sent, orig_us, frames, st):
    """Original timing mode: match by content AND time.

    Many frames are identical (a DoS flood is the same frame thousands of
    times), so content alone cannot say which one arrived. Each received frame
    is matched to the unused identical sent frame closest to the time it was
    expected. The offset between the board's clock and the capture clock is
    learned from the first frames and then tracked, so clock drift between
    the two does not break the matching."""
    from bisect import bisect_left
    by_content = {}
    for j, f in enumerate(sent):
        by_content.setdefault(f, []).append(j)       # in time order
    cand_times = {f: [orig_us[j] for j in js] for f, js in by_content.items()}
    used = [False] * len(sent)

    # Initial offset: median over the first frames matched in order.
    seq, _ = match_sequential(sent[:3000], frames[:300])
    pairs = [st[k] - orig_us[j] for j, k in enumerate(seq) if k >= 0]
    if not pairs:
        return [-1] * len(sent), len(frames)
    offset = float(np.median(pairs))

    rx_for, spurious = [-1] * len(sent), 0
    for k, g in enumerate(frames):
        cands = by_content.get(g)
        if not cands:
            spurious += 1
            continue
        want = st[k] - offset
        times = cand_times[g]
        i = bisect_left(times, want)
        best, lo, hi, inf = -1, i - 1, i, float("inf")
        while True:                             # nearest first, outwards from the expected time
            dl = want - times[lo] if lo >= 0 else inf
            dh = times[hi] - want if hi < len(cands) else inf
            if min(dl, dh) > MATCH_WINDOW_US:
                break
            if dl <= dh:
                x, lo = lo, lo - 1
            else:
                x, hi = hi, hi + 1
            if not used[cands[x]]:
                best = cands[x]
                break
        if best < 0:
            spurious += 1
            continue
        used[best] = True
        rx_for[best] = k
        offset += 0.05 * ((st[k] - orig_us[best]) - offset)   # follow slow clock drift
    return rx_for, spurious


def analyse(rows, frames, stamps, realtime):
    sent = [tuple(int(v) for v in r[1:11]) for r in rows]
    frames, stamps, dups = dedupe(frames, stamps)
    st = unwrap(stamps)
    orig_us = [(r[0] - rows[0][0]) * 1e6 for r in rows]

    if realtime:
        rx_for, spurious = match_timed(sent, orig_us, frames, st)
    else:
        rx_for, spurious = match_sequential(sent, frames)
    arrived = [k >= 0 for k in rx_for]

    first = next((i for i, k in enumerate(rx_for) if k >= 0), None)
    rx_rel, err = [None] * len(sent), [None] * len(sent)
    if first is not None:
        first_rx = st[rx_for[first]]
        for i, k in enumerate(rx_for):
            if k >= 0:
                rx_rel[i] = st[k] - first_rx
                err[i] = rx_rel[i] - (orig_us[i] - orig_us[first])
    # The two ESP32 clocks run at slightly different speeds (tens of ppm), so the
    # raw error grows in a straight line over the run. That slope is the clock
    # drift between the boards, not replay timing. Fit it and keep what is left
    # over as the jitter: how far each frame was from where it should have been.
    jitter, drift_ppm = [None] * len(sent), None
    pts = [(orig_us[i], e) for i, e in enumerate(err) if e is not None]
    if realtime and len(pts) >= 10:
        x = np.array([p[0] for p in pts])
        y = np.array([p[1] for p in pts])
        slope, icpt = np.polyfit(x, y, 1)
        drift_ppm = float(slope * 1e6)
        for i, e in enumerate(err):
            if e is not None:
                jitter[i] = e - (slope * orig_us[i] + icpt)
    return {"arrived": arrived, "spurious": spurious, "duplicates": dups, "rx_rel": rx_rel,
            "err": err, "jitter": jitter, "drift_ppm": drift_ppm, "t0": rows[0][0],
            "realtime": realtime}


def summarise(rows, res, args, extra):
    n = len(rows)
    arrived = np.array(res["arrived"], dtype=bool)
    label = np.array([r[11] for r in rows])
    s = {"file": args.file, "frames_sent": n, "frames_arrived": int(arrived.sum()),
         "frames_lost": int(n - arrived.sum()), "unmatched_received": res["spurious"],
         "duplicates_dropped": res["duplicates"],
         "delivery_percent": round(100.0 * arrived.mean(), 4) if n else 0,
         "mode": "original timing" if res["realtime"] else "fast, timing ignored",
         "capture_duration_s": round(rows[-1][0] - rows[0][0], 6), "by_class": {}}
    for c in sorted(set(label.tolist())):
        m = label == c
        s["by_class"][CLASS_NAMES[c]] = {"sent": int(m.sum()), "arrived": int(arrived[m].sum()),
                                         "lost": int(m.sum() - arrived[m].sum())}
    err = np.array([e for e in res["err"] if e is not None])
    if res["realtime"] and len(err):
        a = np.abs(err)
        s["timing_error_us"] = {"mean": round(float(err.mean()), 1), "p50_abs": round(float(np.percentile(a, 50)), 1),
                                "p95_abs": round(float(np.percentile(a, 95)), 1),
                                "p99_abs": round(float(np.percentile(a, 99)), 1),
                                "max_abs": round(float(a.max()), 1),
                                "over_1ms": int((a > 1000).sum())}
        j = np.abs(np.array([x for x in res["jitter"] if x is not None]))
        if len(j):
            s["timing_jitter_us"] = {"p50": round(float(np.percentile(j, 50)), 1),
                                     "p95": round(float(np.percentile(j, 95)), 1),
                                     "p99": round(float(np.percentile(j, 99)), 1),
                                     "max": round(float(j.max()), 1),
                                     "over_100us": int((j > 100).sum())}
            s["clock_drift_ppm"] = round(res["drift_ppm"], 2)
        rr = [x for x in res["rx_rel"] if x is not None]
        s["replay_duration_s"] = round(rr[-1] / 1e6, 6) if rr else None
    s.update(extra)
    return s


def save_run(out, rows, res, frames, stamps, summary):
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "frames.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["index", "capture_time_s", "can_id", "dlc", "data", "class", "arrived",
                    "received_us", "timing_error_us", "timing_jitter_us"])
        for i, r in enumerate(rows):
            w.writerow([i, "%.6f" % r[0], "%03X" % r[1], r[2], " ".join("%02X" % b for b in r[3:3 + r[2]]),
                        CLASS_NAMES[r[11]], int(res["arrived"][i]),
                        "" if res["rx_rel"][i] is None else int(res["rx_rel"][i]),
                        "" if res["err"][i] is None else round(res["err"][i], 1),
                        "" if res["jitter"][i] is None else round(res["jitter"][i], 1)])
    st = unwrap(stamps)
    with open(os.path.join(out, "received.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["receive_order", "board_time_us", "can_id", "dlc", "data"])
        for k, fr in enumerate(frames):
            w.writerow([k, st[k], "%03X" % fr[0], fr[1], " ".join("%02X" % b for b in fr[2:2 + fr[1]])])
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump(summary, f, indent=2)
    report_html.write(os.path.join(out, "report.html"), summary, rows, res, CLASS_NAMES)


def reanalyse(args):
    """Score an earlier run again from the frames it saved."""
    out = args.reanalyse
    old = json.load(open(os.path.join(out, "summary.json")))
    args.file = old["file"]
    rows = load_timed(os.path.join(args.data, args.file), old["frames_sent"])
    frames, stamps = [], []
    with open(os.path.join(out, "received.csv")) as f:
        for r in csv.DictReader(f):
            data = [int(b, 16) for b in r["data"].split()]
            frames.append((int(r["can_id"], 16), int(r["dlc"])) + tuple(data + [0] * (8 - len(data))))
            stamps.append(int(r["board_time_us"]))
    realtime = old.get("mode", "").startswith("original")
    res = analyse(rows, frames, stamps, realtime)
    keep = {k: old[k] for k in ("started", "sender_report", "receiver_report",
                                "receiver_frames_this_run", "wifi_reconnects") if k in old}
    summary = summarise(rows, res, args, keep)
    save_run(out, rows, res, frames, stamps, summary)
    print("Rebuilt %s: arrived %d of %d" % (out, summary["frames_arrived"], summary["frames_sent"]))
    if "timing_jitter_us" in summary:
        tj = summary["timing_jitter_us"]
        print("Timing, clock drift removed: median %.1f us, 95%% within %.1f us, worst %.1f us; "
              "drift %.1f ppm" % (tj["p50"], tj["p95"], tj["max"], summary["clock_drift_ppm"]))


# ---- run ------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender", help="serial port of ESP32 A, for example COM5")
    ap.add_argument("--receiver", help="serial port of ESP32 B when it is on USB (usb_receiver "
                                      "sketch), for example COM9. Leave out to use WiFi.")
    ap.add_argument("--port", type=int, default=5000, help="TCP port ESP32 B connects to over WiFi")
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--file", default="DoS_dataset.csv", choices=sorted(FILE_CLASS))
    ap.add_argument("--n", type=int, default=20000, help="frames to replay, 0 for the whole file")
    ap.add_argument("--fast", action="store_true", help="ignore the original timing")
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--out", default=None, help="output folder, default runs/<time>_<file>")
    ap.add_argument("--reanalyse", metavar="RUN_FOLDER",
                    help="rebuild the results of an earlier run from its received.csv, no hardware")
    args = ap.parse_args()
    if args.reanalyse:
        reanalyse(args)
        return
    if not args.sender:
        ap.error("--sender is required, for example --sender COM8")

    path = os.path.join(args.data, args.file)
    if not os.path.exists(path):
        sys.exit("%s not found. Put the dataset files in %s" % (args.file, args.data))
    rows = load_timed(path, args.n)
    if not rows:
        sys.exit("No frames read from %s" % path)
    realtime = not args.fast
    span = rows[-1][0] - rows[0][0]
    print("Loaded %d frames from %s (capture spans %.2f s)" % (len(rows), args.file, span))
    if realtime and span > 3600:
        sys.exit("The timestamp counter on the ESP32 wraps after 71 minutes. Use a smaller --n.")

    if args.receiver:
        if args.receiver.upper() == args.sender.upper():
            sys.exit("--sender and --receiver must be two different COM ports")
        rcv = SerialReceiver(E.open_board(args.receiver, args.baud, "Receiver (ESP32 B)"))
        rcv.start()
    else:
        rcv = Receiver(args.port)
        rcv.start()
        print("\nWaiting for ESP32 B to connect on port %d." % args.port)
        print("Put one of these in PC_IP in secrets.h: %s" % ", ".join(local_ips() or ["(run ipconfig)"]))
        print("If Windows asks about the firewall, allow Python on private networks.")
        if not rcv.connected.wait(timeout=300):
            sys.exit("ESP32 B never connected. Check WiFi, PC_IP, and the firewall.")
        print("ESP32 B connected from %s" % rcv.peer)

    # Let the board empty anything it buffered before this run, then start keeping frames.
    t_drain = time.time()
    while time.time() - rcv.last_data < 1.5 and time.time() - t_drain < 20:
        time.sleep(0.1)
    time.sleep(1.2)                       # catch one #STAT line as the starting count
    stat_before = rcv.last_stat()
    if rcv.ignored:
        print("Ignored %d old frames the receiver had buffered before this run" % rcv.ignored)
    rcv.arm()

    tx = E.open_board(args.sender, args.baud, "Sender   (ESP32 A)")
    t_run = time.time()
    packets = [pack_timed(r, rel) for r, rel in zip(rows, start_offsets(rows, realtime))]

    started = time.strftime("%Y-%m-%d %H:%M:%S")
    secs = E.stream(tx, packets)
    print("All frames handed to the sender after %.1f s" % secs)

    tx.write(b"Q")                        # the sender answers once it has sent everything
    txline, t_q, raw = "", time.time(), b""
    while "#TX" not in txline and time.time() - t_q < 30:
        time.sleep(0.2)
        raw += tx.read(tx.in_waiting)
        txline = raw.decode("ascii", "replace").replace("A", "").strip()
    txline = txline[txline.find("#TX"):] if "#TX" in txline else ""

    while time.time() - rcv.last_data < 2.5:
        time.sleep(0.2)
    time.sleep(1.2)                       # one more #STAT after the last frame
    rcv.stop = True
    stat = rcv.last_stat() or "no #STAT line"
    silent = time.time() - rcv.last_stat_time
    if silent > 3:
        stat += " (WARNING: last report %.0f s ago, the receiver stopped talking during the run)" % silent
        print("\nWARNING: ESP32 B stopped reporting %.0f s before the end. Its numbers are stale and "
              "frames after that point were not recorded. Plug it into the PC and open the Serial "
              "Monitor at 115200 to see why." % silent)
    print("Sender says:   %s" % (txline or "no reply"))
    print("Receiver says: %s" % stat)

    res = analyse(rows, rcv.frames, rcv.stamps, realtime)
    rx_run = None
    try:
        a = int(stat_before.split("rx=")[1].split()[0]) if stat_before else None
        b = int(stat.split("rx=")[1].split()[0])
        rx_run = b - a if a is not None else None
    except (IndexError, ValueError):
        pass
    if rx_run is not None:
        stat = "%s (heard %d frames during this run)" % (stat, rx_run)
    summary = summarise(rows, res, args, {"started": started, "sender_report": txline,
                                          "receiver_report": stat,
                                          "receiver_frames_this_run": rx_run,
                                          "wifi_reconnects": max(0, rcv.connections - 1)})
    out = args.out or os.path.join(ROOT, "runs", "%s_%s" % (time.strftime("%Y%m%d_%H%M%S"),
                                                               args.file.replace(".csv", "")))
    save_run(out, rows, res, rcv.frames, rcv.stamps, summary)

    print("\nSent %d, arrived %d, lost %d (%.3f%% delivered)"
          % (summary["frames_sent"], summary["frames_arrived"], summary["frames_lost"],
             summary["delivery_percent"]))
    if "timing_jitter_us" in summary:
        tj = summary["timing_jitter_us"]
        print("Timing against the original capture, clock drift removed: median %.1f us, "
              "95%% within %.1f us, worst %.1f us" % (tj["p50"], tj["p95"], tj["max"]))
        print("Clock drift between the two boards: %.1f ppm" % summary["clock_drift_ppm"])
    elif "timing_error_us" in summary:
        te = summary["timing_error_us"]
        print("Timing against the original capture: typical %.0f us, worst %.0f us, "
              "%d frames off by more than 1 ms" % (te["p50_abs"], te["max_abs"], te["over_1ms"]))
    print("\nEvery frame is saved in:\n  %s" % out)
    print("Open report.html there for the results page.")


if __name__ == "__main__":
    main()
