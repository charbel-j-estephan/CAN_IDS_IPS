"""Replay a Car Hacking dataset file onto a real CAN bus with its original timing,
record every frame the receiver hears, and write a results page.

    PC --USB--> ESP32 A (replay_sender) --CAN bus--> ESP32 B (wifi_receiver) --WiFi--> PC

ESP32 B needs no USB data link. It connects to this script over WiFi (same
hotspot as the PC) and every frame arrives here with a microsecond timestamp.

    python python/replay_dataset.py --sender COM5
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
    """TCP server: ESP32 B connects to it and every frame is stored here."""

    def __init__(self, port):
        super().__init__(daemon=True)
        self.srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.srv.bind(("0.0.0.0", port))
        self.srv.listen(1)
        self.frames, self.stamps, self.texts = [], [], []
        self.buf = bytearray()
        self.connected = threading.Event()
        self.last_data = time.time()       # time of the last FRAME received
        self.stop = False

    def run(self):
        conn, addr = self.srv.accept()
        self.peer = addr[0]
        self.connected.set()
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
            self.buf += data
            f, s, t = parse_timed_stream(self.buf)
            self.frames += f
            self.stamps += s
            self.texts += t
            if f:                          # the once a second #STAT lines do not count
                self.last_data = time.time()
        conn.close()


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


def analyse(rows, frames, stamps, realtime):
    sent = [tuple(int(v) for v in r[1:11]) for r in rows]

    # Frames keep their order on the bus, so walk both lists once: every received
    # frame is matched to the next identical sent frame, and the ones skipped
    # over were lost. rx_for[i] is the received record for sent frame i, or -1.
    rx_for = [-1] * len(sent)
    spurious, p = 0, 0
    for k, g in enumerate(frames):
        end = min(len(sent), p + E.GW_WINDOW)
        j = p
        while j < end and sent[j] != g:
            j += 1
        if j < end:
            rx_for[j] = k
            p = j + 1
        else:
            spurious += 1
    arrived = [k >= 0 for k in rx_for]
    st = unwrap(stamps)
    t0_orig = rows[0][0]
    first_rx = next((st[k] for k in rx_for if k >= 0), None)
    first_orig = next((rows[i][0] for i, k in enumerate(rx_for) if k >= 0), None)

    rx_rel, err = [None] * len(sent), [None] * len(sent)
    for i, k in enumerate(rx_for):
        if k >= 0:
            rx_rel[i] = st[k] - first_rx
            err[i] = rx_rel[i] - (rows[i][0] - first_orig) * 1e6
    return {"arrived": arrived, "spurious": spurious, "rx_rel": rx_rel, "err": err,
            "t0": t0_orig, "realtime": realtime}


def summarise(rows, res, args, extra):
    n = len(rows)
    arrived = np.array(res["arrived"], dtype=bool)
    label = np.array([r[11] for r in rows])
    s = {"file": args.file, "frames_sent": n, "frames_arrived": int(arrived.sum()),
         "frames_lost": int(n - arrived.sum()), "unmatched_received": res["spurious"],
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
        rr = [x for x in res["rx_rel"] if x is not None]
        s["replay_duration_s"] = round(rr[-1] / 1e6, 6) if rr else None
    s.update(extra)
    return s


def save_run(out, rows, res, frames, stamps, summary):
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "frames.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["index", "capture_time_s", "can_id", "dlc", "data", "class", "arrived",
                    "received_us", "timing_error_us"])
        for i, r in enumerate(rows):
            w.writerow([i, "%.6f" % r[0], "%03X" % r[1], r[2], " ".join("%02X" % b for b in r[3:3 + r[2]]),
                        CLASS_NAMES[r[11]], int(res["arrived"][i]),
                        "" if res["rx_rel"][i] is None else int(res["rx_rel"][i]),
                        "" if res["err"][i] is None else round(res["err"][i], 1)])
    st = unwrap(stamps)
    with open(os.path.join(out, "received.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["receive_order", "board_time_us", "can_id", "dlc", "data"])
        for k, fr in enumerate(frames):
            w.writerow([k, st[k], "%03X" % fr[0], fr[1], " ".join("%02X" % b for b in fr[2:2 + fr[1]])])
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump(summary, f, indent=2)
    report_html.write(os.path.join(out, "report.html"), summary, rows, res, CLASS_NAMES)


# ---- run ------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender", required=True, help="serial port of ESP32 A, for example COM5")
    ap.add_argument("--port", type=int, default=5000, help="TCP port ESP32 B connects to")
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--file", default="DoS_dataset.csv", choices=sorted(FILE_CLASS))
    ap.add_argument("--n", type=int, default=20000, help="frames to replay, 0 for the whole file")
    ap.add_argument("--fast", action="store_true", help="ignore the original timing")
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--out", default=None, help="output folder, default runs/<time>_<file>")
    args = ap.parse_args()

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

    rcv = Receiver(args.port)
    rcv.start()
    print("\nWaiting for ESP32 B to connect on port %d." % args.port)
    print("Put one of these in PC_IP in secrets.h: %s" % ", ".join(local_ips() or ["(run ipconfig)"]))
    print("If Windows asks about the firewall, allow Python on private networks.")
    if not rcv.connected.wait(timeout=300):
        sys.exit("ESP32 B never connected. Check WiFi, PC_IP, and the firewall.")
    print("ESP32 B connected from %s" % rcv.peer)

    tx = E.open_board(args.sender, args.baud, "Sender   (ESP32 A)")
    t0 = rows[0][0]
    packets = [pack_timed(r, 0 if not realtime else round((r[0] - t0) * 1e6)) for r in rows]

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
    stat = next((t for t in reversed(rcv.texts) if t.startswith("#STAT")), "no #STAT line")
    print("Sender says:   %s" % (txline or "no reply"))
    print("Receiver says: %s" % stat)

    res = analyse(rows, rcv.frames, rcv.stamps, realtime)
    summary = summarise(rows, res, args, {"started": started, "sender_report": txline,
                                          "receiver_report": stat})
    out = args.out or os.path.join(ROOT, "runs", "%s_%s" % (time.strftime("%Y%m%d_%H%M%S"),
                                                               args.file.replace(".csv", "")))
    save_run(out, rows, res, rcv.frames, rcv.stamps, summary)

    print("\nSent %d, arrived %d, lost %d (%.3f%% delivered)"
          % (summary["frames_sent"], summary["frames_arrived"], summary["frames_lost"],
             summary["delivery_percent"]))
    if "timing_error_us" in summary:
        te = summary["timing_error_us"]
        print("Timing against the original capture: typical %.0f us, worst %.0f us, "
              "%d frames off by more than 1 ms" % (te["p50_abs"], te["max_abs"], te["over_1ms"]))
    print("\nEvery frame is saved in:\n  %s" % out)
    print("Open report.html there for the results page.")


if __name__ == "__main__":
    main()
