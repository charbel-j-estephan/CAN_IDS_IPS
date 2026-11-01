"""Replay a Car Hacking dataset file onto a real CAN bus with its original timing,
record every frame the receiver hears, and write a results page.

    PC --USB--> sender (replay_sender) --CAN bus--> receiver (usb_receiver) --USB--> PC
    PC --USB--> attacker (attack_sender) --+   (optional third board, --attacker)

Every frame arrives with the microsecond time the receiver received it. With
--attacker, the sender sends only the normal frames and the attacker only the
attack frames, as two real nodes. The sender sends a start marker first and the
attacker starts its clock from it. The attacker also gets a timing reference
for every normal frame, so it can follow the sender's
clock for the whole run instead of drifting away from it.

    python replay_dataset.py --sender COM8 --receiver COM7
    python replay_dataset.py --sender COM8 --receiver COM7 --attacker COM9
    python replay_dataset.py --sender COM8 --receiver COM7 --file Fuzzy_dataset.csv --n 100000
    python replay_dataset.py --sender COM8 --receiver COM7 --fast     ignore timing, send flat out
    python replay_dataset.py --reanalyse ../runs/<folder>             rescore a saved run

Everything is saved under runs/<time>_<file>/ : frames.csv holds EVERY frame
sent (with whether it arrived and when), received.csv every frame heard,
summary.json the numbers, report.html the results page.

Needs numpy and pyserial (pip install numpy pyserial).
"""
import argparse
import csv
import json
import os
import sys
import threading
import time

import numpy as np

import report_html
import serial_link
from can_bits import frame_us

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLASS_NAMES = ["Normal", "DoS", "Fuzzy", "Gear_spoof", "RPM_spoof"]
FILE_CLASS = {"DoS_dataset.csv": 1, "Fuzzy_dataset.csv": 2,
              "gear_dataset.csv": 3, "RPM_dataset.csv": 4}
SYNC_TS = 0xA7
SYNC_IDS = 0xA8              # STM32 IDS node: frame record plus its decision
IDS_TICKS_PER_US = 84        # the STM32's TIM4 runs at 84 MHz
IDS_LATE, IDS_OFF = 0xFF, 0xFE
# Start marker for the attacker (must match attack_sender.ino): ID 0x7FF, "CANSTART".
MARKER = (0x7FF, 8) + tuple(b"CANSTART")
GAP_US = 20000               # first dataset frame starts this long after the marker


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
    """15 byte packet for a sender: id, dlc, 8 data bytes, microsecond offset."""
    cid, dlc = row[1], row[2]
    return bytes([cid >> 8, cid & 0xFF, dlc] + [int(b) for b in row[3:11]]) + \
        int(rel_us).to_bytes(4, "little")


def start_offsets(rows, realtime):
    """When the sender must START each frame, in microseconds after the first one.

    The dataset stamps a frame when it ENDS (checked against the capture: the
    gap before a frame is never shorter than that frame's own length). So each
    frame starts one frame length earlier, and that length depends on its
    data through bit stuffing. 0 means "send now"."""
    if not realtime:
        return [0] * len(rows)
    starts = [r[0] * 1e6 - frame_us(r[1], r[2], r[3:3 + r[2]]) for r in rows]
    return [max(0, round(x - starts[0])) for x in starts]


REF_TAG = 0xFF               # first byte of a timing reference packet for the attacker


def ref_packet(row, rel_us):
    """15 byte timing reference for the attacker: a normal frame and when it
    should END on the bus (its start plus its length on the wire)."""
    cid = row[1]
    end = rel_us + frame_us(cid, row[2], row[3:3 + row[2]])
    return bytes([REF_TAG, cid >> 8, cid & 0xFF]) + bytes(8) + int(round(end)).to_bytes(4, "little")


def marker_packet():
    return bytes([MARKER[0] >> 8, MARKER[0] & 0xFF, MARKER[1]] + list(MARKER[2:])) + bytes(4)


def parse_report(line):
    """'#TX sent=5 late=0 state=running' -> {'sent': 5, 'late': 0, 'state': 'running'}."""
    out = {}
    for part in (line or "").split()[1:]:
        if "=" not in part:
            continue
        k, _, v = part.partition("=")
        out[k] = int(v) if v.lstrip("-").isdigit() else v
    return out


class IpsNode(threading.Thread):
    """The STM32 IDS node when it runs prevention (--ips): commands and its #STAT.

    It is not the receiver in an IPS run (the ESP32 logger is, it records what
    survived on the bus); this only switches prevention on and off and reads the
    STM32's counters. Prevention is switched off again however the script ends."""

    def __init__(self, port):
        super().__init__(daemon=True)
        import atexit
        import serial
        self.ser = serial.Serial(port, 115200, timeout=0.2)
        self.lines, self.last, self.stop = [], None, False
        self.lock = threading.Lock()
        atexit.register(self.close)

    def run(self):
        buf = b""
        while not self.stop:
            try:
                buf += self.ser.read(max(1, self.ser.in_waiting))
            except Exception:
                break
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode("ascii", "replace").strip()
                if text.startswith("#STAT"):
                    with self.lock:
                        self.lines.append(text)
                        self.last = text

    def command(self, c):
        self.ser.write(c)
        time.sleep(0.05)

    def wait_stat(self, ok, fresh=False, timeout=5):
        """First #STAT line (arriving from now on if fresh) whose fields pass ok()."""
        with self.lock:
            start = len(self.lines)
        t0 = time.time()
        while time.time() - t0 < timeout:
            with self.lock:
                new = self.lines[start:] if fresh else self.lines[-1:]
            hit = next((l for l in new if ok(parse_report(l))), None)
            if hit:
                return hit
            time.sleep(0.2)
        return None

    def close(self):
        if self.ser.is_open:
            try:
                self.ser.write(b"i")      # prevention off
                time.sleep(0.1)
            except Exception:
                pass
            self.stop = True
            time.sleep(0.3)
            self.ser.close()


def print_ips(summary):
    """What prevention did, from the logger's record of what survived on the bus."""
    by = summary.get("by_class", {})
    att = {k: v for k, v in by.items() if k != "Normal"}
    a_sent = sum(v["sent"] for v in att.values())
    a_lost = sum(v["lost"] for v in att.values())
    n = by.get("Normal", {"sent": 0, "lost": 0})
    print("\nSTM32 IPS (prevention on):")
    if a_sent:
        print("  attack frames destroyed: %d of %d (%.2f%%)" % (a_lost, a_sent, 100.0 * a_lost / a_sent))
        for k, v in att.items():
            print("    %-10s %d of %d" % (k, v["lost"], v["sent"]))
    print("  normal frames lost:      %d of %d%s" % (n["lost"], n["sent"],
          "  <- wrongly destroyed, or lost on the bus" if n["lost"] else ""))
    s = summary.get("nodes", {}).get("STM32 IPS", {})
    if s:
        print("  STM32: %s error flags sent, %s attack verdicts too late (frame got through), "
              "%s allowed (start marker), %s flags cut short, %s frames with no decision in time"
              % (s.get("rej"), s.get("ilate"), s.get("allow"), s.get("abort"), s.get("dlate")))
    a = summary.get("nodes", {}).get("attacker", {})
    if a:
        print("  attacker: state %s, transmit error counter %s, failed sends %s"
              % (a.get("state"), a.get("tec"), a.get("txfail")))
    print("  (A destroyed frame is retransmitted by its sender until it gets through or the "
          "sender goes bus off, so 'error flags sent' can be larger than 'destroyed'.)")


def ask_report(ser, timeout=30):
    """Ask a sender for its final #TX line (it answers once everything is sent)."""
    ser.write(b"Q")
    raw, t0 = b"", time.time()
    while time.time() - t0 < timeout:
        i = raw.find(b"#TX")
        if i >= 0 and b"\n" in raw[i:]:       # the whole line has arrived
            break
        time.sleep(0.2)
        raw += ser.read(ser.in_waiting)
    text = raw.decode("ascii", "replace").replace("A", "")
    i = text.find("#TX")
    return text[i:].splitlines()[0].strip() if i >= 0 else ""


# ---- receiving ------------------------------------------------------------------

def parse_timed_stream(buf):
    """Records are 0xA7 id_hi id_lo dlc d0..d7 t0..t3 (16 bytes, the receiver),
    or 0xA8 with the same 16 bytes plus the STM32 IDS decision (23 bytes):
    cls ready(2) decided(2) end(2), TIM4 ticks after the start of frame.
    Lines start with '#'.

    Returns ([(can_id, dlc, d0..d7)], [timestamp us], [text lines],
    [IDS decision (cls, ready, decided, end) or None])."""
    frames, stamps, texts, dets = [], [], [], []
    i, n = 0, len(buf)
    while i < n:
        b = buf[i]
        if b in (SYNC_TS, SYNC_IDS):
            size = 16 if b == SYNC_TS else 23
            if i + size > n:
                break
            hi, lo, dlc = buf[i + 1], buf[i + 2], buf[i + 3]
            if hi < 8 and dlc <= 8:
                frames.append((hi << 8 | lo, dlc) + tuple(buf[i + 4:i + 12]))
                stamps.append(int.from_bytes(buf[i + 12:i + 16], "little"))
                if b == SYNC_IDS:
                    dets.append((buf[i + 16],) + tuple(
                        int.from_bytes(buf[i + k:i + k + 2], "little") for k in (17, 19, 21)))
                else:
                    dets.append(None)
                i += size
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
    return frames, stamps, texts, dets


class SerialReceiver(threading.Thread):
    """Reads the receiver (usb_receiver sketch) in the background.

    Frames are only kept once arm() is called. Anything earlier is frames the
    board heard before this run and would otherwise be mistaken for part of it.
    """

    def __init__(self, ser):
        super().__init__(daemon=True)
        self.ser = ser
        self.frames, self.stamps, self.texts, self.dets = [], [], [], []
        self.armed = False
        self.ignored = 0                   # frames received before arm()
        self.last_data = time.time()       # time of the last FRAME received
        self.last_stat_time = 0.0          # the board sends #STAT once a second while alive
        self.stop = False

    def arm(self):
        self.armed = True
        self.last_data = time.time()

    def last_stat(self):
        return next((t for t in reversed(self.texts) if t.startswith("#STAT")), None)

    def run(self):
        buf = bytearray()
        while not self.stop:
            data = self.ser.read(max(1, self.ser.in_waiting))
            if not data:
                continue
            buf += data
            f, s, t, d = parse_timed_stream(buf)
            self.texts += t
            if any(x.startswith("#STAT") for x in t):
                self.last_stat_time = time.time()
            if not f:                      # the once a second #STAT lines do not count
                continue
            self.last_data = time.time()
            if self.armed:
                self.frames += f
                self.stamps += s
                self.dets += d
            else:
                self.ignored += len(f)


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
CLEAR_US = 100               # bus idle this long before a frame: nothing could delay it


def dedupe(frames, stamps, dets=None):
    """Drop records the receiver reported twice.

    Two different frames can never share one microsecond on a 500 kbit/s bus,
    so the same timestamp and ID means the same frame. dets (the STM32's
    decisions, one per record) is thinned the same way."""
    dets = dets if dets is not None else [None] * len(frames)
    seen, f2, s2, d2 = set(), [], [], []
    for f, t, d in zip(frames, stamps, dets):
        key = (t, f[0])
        if key in seen:
            continue
        seen.add(key)
        f2.append(f)
        s2.append(t)
        d2.append(d)
    return f2, s2, len(frames) - len(f2), d2


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


def match_timed(sent, orig_us, frames, st, node=None):
    """Original timing mode: match by content AND time.

    Many frames are identical (a DoS flood is the same frame thousands of
    times), so content alone cannot say which one arrived. Each received frame
    is matched to the unused identical sent frame closest to the time it was
    expected. The offset between the board's clock and the capture clock is
    learned from the first frames and then tracked, so clock drift between
    the two does not break the matching.

    One node sends its identical frames in order, so they also arrive in
    order. Nearest in time can still pair two of them crosswise (one late
    frame lands nearer the next one's time), so afterwards each node's
    identical frames are paired in order. node[j] says which board sent
    frame j (None: one board sent everything)."""
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

    groups = {}
    for j, k in enumerate(rx_for):
        if k >= 0:
            groups.setdefault((sent[j], node[j] if node else 0), []).append(j)
    for js in groups.values():
        for j, k in zip(js, sorted(rx_for[j] for j in js)):
            rx_for[j] = k
    return rx_for, spurious


def analyse(rows, frames, stamps, realtime, split=False, dets=None):
    sent = [tuple(int(v) for v in r[1:11]) for r in rows]
    frames, stamps, dups, dets = dedupe(frames, stamps, dets)
    st = unwrap(stamps)
    orig_us = [(r[0] - rows[0][0]) * 1e6 for r in rows]

    if realtime:
        node = [int(r[11] != 0) for r in rows] if split else None
        rx_for, spurious = match_timed(sent, orig_us, frames, st, node)
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
    # With an attacker board, attack frames run on that board's clock, so each
    # sender gets its own fit.
    jitter, fits = [None] * len(sent), {}
    groups = {0: [i for i in range(len(rows)) if not split or rows[i][11] == 0]}
    if split:
        groups[1] = [i for i in range(len(rows)) if rows[i][11] != 0]
    for g, idx in groups.items():
        pts = [(orig_us[i], err[i]) for i in idx if err[i] is not None]
        if not realtime or len(pts) < 10:
            continue
        x = np.array([p[0] for p in pts])
        y = np.array([p[1] for p in pts])
        slope, icpt = np.polyfit(x, y, 1)
        fits[g] = (slope, icpt)
        for i in idx:
            if err[i] is not None:
                jitter[i] = err[i] - (slope * orig_us[i] + icpt)
    drift_ppm = float(fits[0][0] * 1e6) if 0 in fits else None
    attacker = None
    if 0 in fits and 1 in fits:
        # How far the attacker sends from where the sender's clock says it should: only
        # frames with a clear bus before them in the capture count. A frame
        # that queued behind another one is late for another reason, and the
        # normal frames the attacker pushes aside would otherwise hide part of the offset.
        starts = [orig_us[i] - frame_us(r[1], r[2], r[3:3 + r[2]]) for i, r in enumerate(rows)]
        on_a = lambda i: err[i] - (fits[0][0] * orig_us[i] + fits[0][1])
        clear = {g: [on_a(i) for i in idx if i > 0 and err[i] is not None
                     and starts[i] - orig_us[i - 1] >= CLEAR_US] for g, idx in groups.items()}
        if len(clear[0]) >= 10 and len(clear[1]) >= 10:
            offset = float(np.median(clear[1]) - np.median(clear[0]))
        else:
            offset = float(fits[1][1] - fits[0][1])
        attacker = {"drift_ppm": float(fits[1][0] * 1e6), "offset_us": offset}
    return {"arrived": arrived, "spurious": spurious, "duplicates": dups, "rx_rel": rx_rel,
            "err": err, "jitter": jitter, "drift_ppm": drift_ppm, "attacker_fit": attacker,
            "t0": rows[0][0], "realtime": realtime, "split": split,
            "det": [dets[k] if k >= 0 else None for k in rx_for]}


def ids_summary(label, det):
    """Score the STM32 IDS node's decisions against the dataset labels.

    det[i] is (cls, ready, decided, end) for sent frame i, TIM4 ticks after its
    start of frame edge, or None when that frame did not arrive (or came from a
    receiver without a model)."""
    have = [i for i, d in enumerate(det) if d is not None]
    if not have:
        return None
    cls = np.array([det[i][0] for i in have])
    y = np.array([label[i] for i in have])
    t = np.array([det[i][1:] for i in have], dtype=float) / IDS_TICKS_PER_US
    off = cls == IDS_OFF
    if off.all():
        return {"model": "off", "frames": len(have)}
    late = cls == IDS_LATE
    ok = ~late & ~off
    cm = [[int(((y == a) & (cls == b) & ok).sum()) for b in range(len(CLASS_NAMES))]
          for a in range(len(CLASS_NAMES))]
    s = {"model": "on", "frames": len(have), "no_decision_by_frame_end": int(late.sum()),
         "model_off_frames": int(off.sum()), "confusion": cm, "classes": CLASS_NAMES,
         "recall": {}, "attack_frames": int((y != 0).sum()), "normal_frames": int((y == 0).sum())}
    for c, name in enumerate(CLASS_NAMES):
        m = (y == c) & ~off
        if m.any():
            s["recall"][name] = round(100.0 * float(((cls == c) & m).sum()) / m.sum(), 3)
    att, nor = (y != 0) & ~off, (y == 0) & ~off
    if att.any():   # a late decision counts as a miss
        s["attacks_flagged_percent"] = round(100.0 * float((ok & att & (cls != 0)).sum()) / att.sum(), 3)
    if nor.any():
        fa = int((ok & nor & (cls != 0)).sum())
        s["false_alarms"] = fa
        s["false_alarm_percent"] = round(100.0 * fa / nor.sum(), 4)
    s["accuracy_percent"] = round(100.0 * float(((cls == y) & ok).sum()) / (~off).sum(), 4)
    if ok.any():
        after = t[ok, 1] - t[ok, 0]          # decision after the features were complete
        left = t[ok, 2] - t[ok, 1]           # decision before the 6th end of frame bit
        pc = lambda a, p: round(float(np.percentile(a, p)), 2)
        s["decision_after_features_us"] = {"p50": pc(after, 50), "p99": pc(after, 99),
                                           "max": round(float(after.max()), 2)}
        s["decision_from_frame_start_us"] = {"p50": pc(t[ok, 1], 50), "max": round(float(t[ok, 1].max()), 2)}
        s["time_left_to_frame_end_us"] = {"min": round(float(left.min()), 2), "p1": pc(left, 1),
                                          "p50": pc(left, 50)}
    return s


def print_ids(s):
    if not s:
        return
    if s["model"] == "off":
        print("STM32 IDS: model switched off, %d frames not classified" % s["frames"])
        return
    print("\nSTM32 IDS, decisions on %d frames (%d attack, %d normal):"
          % (s["frames"], s["attack_frames"], s["normal_frames"]))
    print("  recall: " + ", ".join("%s %.3f%%" % kv for kv in s["recall"].items()))
    if "attacks_flagged_percent" in s:
        print("  attack frames flagged: %.3f%%" % s["attacks_flagged_percent"])
    if "false_alarms" in s:
        print("  false alarms: %d (%.4f%% of normal frames)" % (s["false_alarms"], s["false_alarm_percent"]))
    print("  no decision by the end of the frame: %d" % s["no_decision_by_frame_end"])
    if "decision_after_features_us" in s:
        a, l = s["decision_after_features_us"], s["time_left_to_frame_end_us"]
        print("  decision after the 5th data byte: median %.2f us, 99%% within %.2f us, worst %.2f us"
              % (a["p50"], a["p99"], a["max"]))
        print("  time still left before the frame ended: at least %.2f us (median %.2f us)"
              % (l["min"], l["p50"]))


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
            if res.get("attacker_fit"):
                s["attacker_clock_drift_ppm"] = round(res["attacker_fit"]["drift_ppm"], 2)
                s["attacker_sync_offset_us"] = round(res["attacker_fit"]["offset_us"], 1)
        rr = [x for x in res["rx_rel"] if x is not None]
        s["replay_duration_s"] = round(rr[-1] / 1e6, 6) if rr else None
    ids = ids_summary(label, res.get("det", []))
    if ids:
        s["ids"] = ids
    s.update(extra)
    return s


def ids_class_name(cls):
    return {IDS_LATE: "late", IDS_OFF: "off"}.get(cls, CLASS_NAMES[cls] if cls < len(CLASS_NAMES) else str(cls))


def save_run(out, rows, res, frames, stamps, summary, dets=None):
    os.makedirs(out, exist_ok=True)
    det = res.get("det") or [None] * len(rows)
    with open(os.path.join(out, "frames.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["index", "capture_time_s", "can_id", "dlc", "data", "class", "arrived",
                    "received_us", "timing_error_us", "timing_jitter_us",
                    "ids_class", "ids_decided_after_features_us", "ids_time_left_us"])
        for i, r in enumerate(rows):
            d = det[i]
            ids = ["", "", ""]
            if d is not None:
                ids[0] = ids_class_name(d[0])
                if d[0] < len(CLASS_NAMES):
                    ids[1] = round((d[2] - d[1]) / IDS_TICKS_PER_US, 2)
                    ids[2] = round((d[3] - d[2]) / IDS_TICKS_PER_US, 2)
            w.writerow([i, "%.6f" % r[0], "%03X" % r[1], r[2], " ".join("%02X" % b for b in r[3:3 + r[2]]),
                        CLASS_NAMES[r[11]], int(res["arrived"][i]),
                        "" if res["rx_rel"][i] is None else int(res["rx_rel"][i]),
                        "" if res["err"][i] is None else round(res["err"][i], 1),
                        "" if res["jitter"][i] is None else round(res["jitter"][i], 1)] + ids)
    st = unwrap(stamps)
    dets = dets if dets is not None else [None] * len(frames)
    with open(os.path.join(out, "received.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["receive_order", "board_time_us", "can_id", "dlc", "data",
                    "ids_cls", "ids_ready_ticks", "ids_decided_ticks", "ids_end_ticks"])
        for k, fr in enumerate(frames):
            w.writerow([k, st[k], "%03X" % fr[0], fr[1], " ".join("%02X" % b for b in fr[2:2 + fr[1]])]
                       + (list(dets[k]) if dets[k] is not None else ["", "", "", ""]))
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump(summary, f, indent=2)
    report_html.write(os.path.join(out, "report.html"), summary, rows, res, CLASS_NAMES)


def first_normal(rows):
    """With an attacker the run starts at the first normal frame: the attacker
    needs the sender's marker before it can send anything."""
    return next((i for i, r in enumerate(rows) if r[11] == 0), 0)


def rows_from(all_rows, old):
    i = old.get("start_frame", 0)
    return all_rows[i:i + old["frames_sent"]]


def reanalyse(args):
    """Score an earlier run again from the frames it saved."""
    out = args.reanalyse
    old = json.load(open(os.path.join(out, "summary.json")))
    args.file = old["file"]
    rows = load_timed(os.path.join(args.data, args.file), old["frames_sent"])
    frames, stamps, dets = [], [], []
    with open(os.path.join(out, "received.csv")) as f:
        for r in csv.DictReader(f):
            data = [int(b, 16) for b in r["data"].split()]
            frames.append((int(r["can_id"], 16), int(r["dlc"])) + tuple(data + [0] * (8 - len(data))))
            stamps.append(int(r["board_time_us"]))
            ids = [r.get(k) for k in ("ids_cls", "ids_ready_ticks", "ids_decided_ticks", "ids_end_ticks")]
            dets.append(tuple(int(v) for v in ids) if all(ids) else None)
    realtime = old.get("mode", "").startswith("original")
    split = bool(old.get("attacker_report"))
    if split:
        rows = rows_from(load_timed(os.path.join(args.data, args.file),
                                    old.get("start_frame", 0) + old["frames_sent"]), old)
    res = analyse(rows, frames, stamps, realtime, split=split, dets=dets)
    keep = {k: old[k] for k in ("started", "sender_report", "receiver_report", "attacker_report",
                                "nodes", "receiver_frames_this_run", "start_frame") if k in old}
    summary = summarise(rows, res, args, keep)
    save_run(out, rows, res, frames, stamps, summary, dets)
    print("Rebuilt %s: arrived %d of %d" % (out, summary["frames_arrived"], summary["frames_sent"]))
    if "timing_jitter_us" in summary:
        tj = summary["timing_jitter_us"]
        print("Timing, clock drift removed: median %.1f us, 95%% within %.1f us, worst %.1f us; "
              "drift %.1f ppm" % (tj["p50"], tj["p95"], tj["max"], summary["clock_drift_ppm"]))
    print_ids(summary.get("ids"))


# ---- run ------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender", help="COM port of the sender (replay_sender), for example COM8")
    ap.add_argument("--receiver", help="COM port of the receiver (usb_receiver), for example COM7")
    ap.add_argument("--attacker", help="COM port of the attacker (attack_sender): the sender sends "
                                      "the normal frames, the attacker the attack frames")
    ap.add_argument("--ips", metavar="COM",
                    help="COM port of the STM32 IDS node: switch its prevention on for this run "
                         "(it destroys the frames its model calls attacks) and read its counters. "
                         "The receiver stays the ESP32 logger, the record of what survived")
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
    if not (args.sender and args.receiver):
        ap.error("--sender and --receiver are required, for example --sender COM8 --receiver COM7")
    ports = [p.upper() for p in (args.sender, args.receiver, args.attacker, args.ips) if p]
    if len(set(ports)) != len(ports):
        sys.exit("--sender, --receiver, --attacker and --ips must be different COM ports")
    if args.attacker and args.fast:
        sys.exit("--fast cannot be used with --attacker: the attacker needs the original timing")

    path = os.path.join(args.data, args.file)
    if not os.path.exists(path):
        sys.exit("%s not found. Put the dataset files in %s" % (args.file, args.data))
    rows = load_timed(path, args.n)
    if not rows:
        sys.exit("No frames read from %s" % path)
    start_frame = 0
    if args.attacker:
        start_frame = first_normal(rows)
        if start_frame:
            more = load_timed(path, args.n + start_frame if args.n else 0)
            rows = more[start_frame:start_frame + len(rows)] if args.n else more[start_frame:]
    realtime = not args.fast
    span = rows[-1][0] - rows[0][0]
    print("Loaded %d frames from %s (capture spans %.2f s)" % (len(rows), args.file, span))
    if realtime and span > 3600:
        sys.exit("The timestamp counter on the ESP32 wraps after 71 minutes. Use a smaller --n.")

    rcv = SerialReceiver(serial_link.open_board(args.receiver, args.baud, "Receiver",
                                                ready=(b"READY", b"#STAT")))
    rcv.start()

    # Let the receiver empty anything it heard before this run, then start keeping frames.
    t_drain = time.time()
    while time.time() - rcv.last_data < 1.5 and time.time() - t_drain < 20:
        time.sleep(0.1)
    time.sleep(1.2)                       # catch one #STAT line as the starting count
    stat_before = rcv.last_stat()
    if rcv.ignored:
        print("Ignored %d old frames the receiver had buffered before this run" % rcv.ignored)
    rcv.arm()

    ips = None
    if args.ips:
        ips = IpsNode(args.ips)
        ips.start()
        ips.command(b"Z")                 # zero its counters: this run only
        ips.command(b"I")                 # prevention on
        line = ips.wait_stat(lambda p: p.get("ips") == "on")
        if not line:
            ips.close()
            sys.exit("The STM32 on %s did not confirm prevention on (no #STAT with ips=on). "
                     "Is it running stm32_ids fw 8 or later?" % args.ips)
        print("STM32 IPS on %s: prevention on (fw %s)" % (args.ips, parse_report(line).get("fw")))

    offsets = start_offsets(rows, realtime)
    started = time.strftime("%Y-%m-%d %H:%M:%S")
    atk, atkline = None, None
    if not args.attacker:
        tx = serial_link.open_board(args.sender, args.baud, "Sender")
        packets = [pack_timed(r, rel) for r, rel in zip(rows, offsets)]
        secs = serial_link.stream(tx, packets)
        print("All frames handed to the sender after %.1f s" % secs)
    else:
        # The attacker must be listening before the sender sends the marker.
        atk = serial_link.open_board(args.attacker, args.baud, "Attacker")
        tx = serial_link.open_board(args.sender, args.baud, "Sender")
        normal = [marker_packet()] + [pack_timed(r, rel + GAP_US)
                                      for r, rel in zip(rows, offsets) if r[11] == 0]
        # The attacker gets its attack frames and, in file order between them, a
        # reference for every normal frame (the order the sender sends them in),
        # to follow the sender's clock.
        attack = [ref_packet(r, rel + GAP_US) if r[11] == 0 else pack_timed(r, rel + GAP_US)
                  for r, rel in zip(rows, offsets)]
        print("Normal frames: %d to the sender, attack frames: %d to the attacker (plus a timing "
              "reference for each normal frame)" % (len(normal) - 1, len(attack) - len(normal) + 1))
        th = threading.Thread(target=serial_link.stream, args=(atk, attack),
                              kwargs={"name": "attacker"}, daemon=True)
        th.start()
        secs = serial_link.stream(tx, normal, name="sender")
        th.join()
        print("All frames handed to the senders after %.1f s" % secs)
        atkline = ask_report(atk)
    txline = ask_report(tx)

    while time.time() - rcv.last_data < 2.5:
        time.sleep(0.2)
    time.sleep(1.2)                       # one more #STAT after the last frame
    rcv.stop = True
    ipsline = None
    if ips:
        ipsline = ips.wait_stat(lambda p: True, fresh=True) or ips.last
        ips.close()                       # switches prevention off again
        print("STM32 IPS says: %s" % (ipsline or "no reply"))
    stat = rcv.last_stat() or "no #STAT line"
    silent = time.time() - rcv.last_stat_time
    if silent > 3:
        stat += " (WARNING: last report %.0f s ago, the receiver stopped talking during the run)" % silent
        print("\nWARNING: the receiver stopped reporting %.0f s before the end. Its numbers are stale and "
              "frames after that point were not recorded." % silent)
    print("Sender says:   %s" % (txline or "no reply"))
    if args.attacker:
        print("Attacker says: %s" % (atkline or "no reply"))
        ar = parse_report(atkline)
        if "tracked" in ar:
            print("Attacker followed the sender's clock: %s frames matched, %s missed, "
                  "shifted %s us by the end (%.1f ppm)"
                  % (ar["tracked"], ar.get("missed"), ar.get("skew"), int(ar.get("drift", 0)) / 1000))
        if ar.get("synced") == 0:
            print("WARNING: the attacker never heard the start marker, so it sent nothing. "
                  "Is it on the same bus?")
    print("Receiver says: %s" % stat)

    frames, stamps, dets = rcv.frames, rcv.stamps, rcv.dets
    if args.attacker:                     # the start marker is not part of the dataset
        keep = [k for k, f in enumerate(frames) if f != MARKER]
        frames, stamps, dets = ([frames[k] for k in keep], [stamps[k] for k in keep],
                                [dets[k] for k in keep])
    res = analyse(rows, frames, stamps, realtime, split=bool(args.attacker), dets=dets)
    rx_run = None
    try:
        a = int(stat_before.split("rx=")[1].split()[0]) if stat_before else None
        b = int(stat.split("rx=")[1].split()[0])
        rx_run = b - a if a is not None else None
    except (IndexError, ValueError):
        pass
    if rx_run is not None:
        stat = "%s (heard %d frames during this run)" % (stat, rx_run)
    nodes = {"sender": parse_report(txline)}
    if args.attacker:
        nodes["attacker"] = parse_report(atkline)
    nodes["receiver"] = parse_report(stat.replace("#STAT", "#TX", 1))
    extra = {"started": started, "sender_report": txline, "receiver_report": stat,
             "receiver_frames_this_run": rx_run, "nodes": nodes, "start_frame": start_frame}
    if args.attacker:
        extra["attacker_report"] = atkline
    if ips:
        nodes["STM32 IPS"] = parse_report(ipsline)
        extra["ips_report"] = ipsline
    summary = summarise(rows, res, args, extra)
    out = args.out or os.path.join(ROOT, "runs", "%s_%s" % (time.strftime("%Y%m%d_%H%M%S"),
                                                               args.file.replace(".csv", "")))
    save_run(out, rows, res, frames, stamps, summary, dets)

    print("\nSent %d, arrived %d, lost %d (%.3f%% delivered)"
          % (summary["frames_sent"], summary["frames_arrived"], summary["frames_lost"],
             summary["delivery_percent"]))
    if "timing_jitter_us" in summary:
        tj = summary["timing_jitter_us"]
        print("Timing against the original capture, clock drift removed: median %.1f us, "
              "95%% within %.1f us, worst %.1f us" % (tj["p50"], tj["p95"], tj["max"]))
        print("Clock drift between the two boards: %.1f ppm" % summary["clock_drift_ppm"])
    if "attacker_sync_offset_us" in summary:
        print("Attacker: clock drift %.1f ppm, sends %.1f us from the sender's clock "
              "(frames with a clear bus before them)"
              % (summary["attacker_clock_drift_ppm"], summary["attacker_sync_offset_us"]))
    print_ids(summary.get("ids"))
    if ips:
        print_ips(summary)
    print("\nEvery frame is saved in:\n  %s" % out)
    print("Open report.html there for the results page.")


if __name__ == "__main__":
    main()
