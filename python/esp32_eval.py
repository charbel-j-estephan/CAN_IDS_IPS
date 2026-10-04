"""Run the IDS across two real ESP32s and measure its accuracy.

PC --USB--> ESP32 A (sender) --CAN bus--> ESP32 B (receiver + forest) --USB--> PC

The PC takes the held out test frames that train.py saved (never seen in
training), streams them through ESP32 A onto the bus, and records the class
that ESP32 B's copy of the forest gives each frame. It then compares that class
with the true label from the dataset.

    python python/train.py  --data data/car_hacking
    python python/export_c.py
    python python/esp32_eval.py --copy-model         # puts the model in the receiver sketch
    (flash esp32/sender and esp32/receiver with the Arduino IDE)
    python python/esp32_eval.py --sender COM5 --receiver COM6 --n 20000

    python python/esp32_eval.py --selftest           # no hardware, checks the scoring code

Needs pyserial for the real run (pip install pyserial).
"""
import argparse
import os
import re
import shutil
import sys
import threading
import time

import numpy as np

SYNC = 0xA5          # first byte of every record from the receiver
SYNC_FRAME = 0xA6    # first byte of a pass-through record: id_hi id_lo dlc d0..d7
GW_WINDOW = 100000   # how far ahead to look for a frame in gateway mode (long attack floods)
PACKET = 11          # bytes per frame sent to ESP32 A
ACK_EVERY = 64       # ESP32 A sends 'A' after this many frames
MAX_INFLIGHT = 256   # frames the PC may have sent but ESP32 A has not acked
SEARCH = 64          # how far ahead to look for a frame after a loss
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


# ---- pure helpers, no hardware ------------------------------------------------

def pack_frame(row):
    """Feature row (can_id, dlc, d0..d7) -> the 11 byte packet ESP32 A expects."""
    can_id = int(row[0]) & 0x7FF
    return bytes([can_id >> 8, can_id & 0xFF, int(row[1])] + [int(b) for b in row[2:10]])


def parse_stream(buf):
    """Pull records and '#' text lines out of buf, leave any partial tail in it.

    A record is 0xA5, class, id_hi, id_lo. Returns ([(class, can_id)], [text]).
    """
    records, texts = [], []
    i, n = 0, len(buf)
    while i < n:
        b = buf[i]
        if b == SYNC:
            if i + 4 > n:
                break
            cls, hi, lo = buf[i + 1], buf[i + 2], buf[i + 3]
            if cls < 8 and hi < 8:
                records.append((cls, hi << 8 | lo))
                i += 4
                continue
            i += 1
        elif b == 0x23:   # '#'
            j = buf.find(b"\n", i)
            if j < 0:
                break
            texts.append(bytes(buf[i:j]).decode("ascii", "replace"))
            i = j + 1
        else:
            i += 1
    del buf[:i]
    return records, texts


def align(sent_ids, rec_ids, window=SEARCH):
    """Match each received frame to the frame that was sent.

    The bus has one transmitter, so frames arrive in order. If one is lost, the
    next record's CAN ID shows up a few positions ahead in the sent list; skip
    to it and count the gap as lost. Returns (sent index per record or -1,
    number of sent frames never seen).
    """
    p, idx, lost = 0, [], 0
    for rid in rec_ids:
        if p < len(sent_ids) and sent_ids[p] == rid:
            idx.append(p)
            p += 1
            continue
        found = -1
        for j in range(p + 1, min(p + 1 + window, len(sent_ids))):
            if sent_ids[j] == rid:
                found = j
                break
        if found >= 0:
            lost += found - p
            idx.append(found)
            p = found + 1
        else:
            idx.append(-1)
    lost += len(sent_ids) - p
    return idx, lost


def parse_frame_stream(buf):
    """Like parse_stream, for the pass-through receiver.

    A record is 0xA6, id_hi, id_lo, dlc, d0..d7 (12 bytes). Returns
    ([(can_id, dlc, d0..d7)], [text]) with data bytes past the DLC as 0.
    """
    frames, texts = [], []
    i, n = 0, len(buf)
    while i < n:
        b = buf[i]
        if b == SYNC_FRAME:
            if i + 12 > n:
                break
            hi, lo, dlc = buf[i + 1], buf[i + 2], buf[i + 3]
            if hi < 8 and dlc <= 8:
                data = [buf[i + 4 + k] if k < dlc else 0 for k in range(8)]
                frames.append((hi << 8 | lo, dlc) + tuple(data))
                i += 12
                continue
            i += 1
        elif b == 0x23:   # '#'
            j = buf.find(b"\n", i)
            if j < 0:
                break
            texts.append(bytes(buf[i:j]).decode("ascii", "replace"))
            i = j + 1
        else:
            i += 1
    del buf[:i]
    return frames, texts


def align_frames(sent, got, window=GW_WINDOW):
    """Which sent frames reached the far side of the gateway?

    Frames keep their order, and the gateway may drop any of them. Walk both
    lists: each received frame is matched to the next sent frame that is
    identical (CAN ID, DLC and data). Everything skipped over was dropped.
    Returns (arrived flag per sent frame, received frames that matched nothing).
    """
    arrived = [False] * len(sent)
    p, spurious = 0, 0
    for g in got:
        end = min(len(sent), p + window)
        j = p
        while j < end and sent[j] != g:
            j += 1
        if j < end:
            arrived[j] = True
            p = j + 1
        else:
            spurious += 1
    return arrived, spurious


def report_gateway(y_true, passed, y_python, names):
    """Score a gateway: it should pass Normal frames and block every attack."""
    y_true = np.asarray(y_true)
    passed = np.asarray(passed, dtype=bool)
    attack = y_true != 0
    blocked = ~passed
    n = len(y_true)
    print("\nFrames sent: %d" % n)
    print("ACCURACY (right decision per frame): %.4f%%"
          % (100 * ((passed & ~attack) | (blocked & attack)).mean()))
    if y_python is not None:
        print("Agreement with the Python model's decision: %.4f%%"
              % (100 * (blocked == (np.asarray(y_python) != 0)).mean()))
    print("\n%-12s %8s %9s %9s" % ("class", "frames", "blocked", "passed"))
    for c, nm in enumerate(names):
        m = y_true == c
        if m.any():
            print("%-12s %8d %8.2f%% %8.2f%%" % (nm, m.sum(), 100 * blocked[m].mean(),
                                                100 * passed[m].mean()))
    if attack.any():
        print("\nAttack frames blocked (detection rate):  %.4f%%" % (100 * blocked[attack].mean()))
    if (~attack).any():
        print("Normal frames blocked (false positives): %.4f%%" % (100 * blocked[~attack].mean()))


def confusion(y_true, y_pred, n_classes):
    cm = np.zeros((n_classes, n_classes), dtype=np.int64)
    np.add.at(cm, (y_true, y_pred), 1)
    return cm


def report(y_true, y_pred, y_python, names):
    """Print accuracy, per class numbers, confusion matrix and attack detection."""
    k = len(names)
    cm = confusion(y_true, y_pred, k)
    n = len(y_true)
    acc = (y_true == y_pred).mean()
    print("\nFrames scored: %d" % n)
    print("ACCURACY (ESP32 vs true label):      %.4f%%" % (100 * acc))
    print("Agreement (ESP32 vs Python model):   %.4f%%"
          % (100 * (y_pred == y_python).mean()))
    print("Accuracy of the Python model itself: %.4f%%\n"
          % (100 * (y_python == y_true).mean()))

    print("%-12s %9s %9s %9s %9s" % ("class", "precision", "recall", "f1", "support"))
    for c in range(k):
        tp = cm[c, c]
        prec = tp / cm[:, c].sum() if cm[:, c].sum() else float("nan")
        rec = tp / cm[c].sum() if cm[c].sum() else float("nan")
        f1 = 2 * prec * rec / (prec + rec) if prec + rec > 0 else float("nan")
        print("%-12s %9.4f %9.4f %9.4f %9d" % (names[c], prec, rec, f1, cm[c].sum()))

    print("\nConfusion matrix (rows = true, columns = predicted)")
    print("%-12s" % "" + "".join("%11s" % nm[:10] for nm in names))
    for c in range(k):
        print("%-12s" % names[c] + "".join("%11d" % v for v in cm[c]))

    is_attack, flagged = y_true != 0, y_pred != 0
    if is_attack.any() and (~is_attack).any():
        print("\nAs an alarm (any attack class vs Normal):")
        print("  detection rate      %.4f%%  (attacks flagged)" % (100 * flagged[is_attack].mean()))
        print("  false positive rate %.4f%%  (normal frames flagged)" % (100 * flagged[~is_attack].mean()))
    return acc


# ---- hardware -----------------------------------------------------------------

def open_board(port, baud, label):
    import serial
    ser = serial.Serial(port, baud, timeout=0.1)
    # Same reset sequence as esptool: pulse EN so the sketch starts from scratch.
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setRTS(False)
    seen, t0 = b"", time.time()
    while b"READY" not in seen:
        if time.time() - t0 > 20:
            sys.exit("%s on %s never printed READY. Is the sketch flashed and the "
                     "baud rate 921600? If it prints 'CAN init failed', check the wiring."
                     % (label, port))
        seen += ser.read(256)
    ser.reset_input_buffer()
    print("%s ready on %s" % (label, port))
    return ser


class Collector(threading.Thread):
    """Reads the receiver's serial port in the background."""

    def __init__(self, ser, frames=False):
        super().__init__(daemon=True)
        self.parse = parse_frame_stream if frames else parse_stream
        self.ser, self.buf = ser, bytearray()
        self.records, self.texts = [], []
        self.last_data = time.time()
        self.stop = False

    def run(self):
        while not self.stop:
            data = self.ser.read(max(1, self.ser.in_waiting))
            if data:
                self.buf += data
                r, t = self.parse(self.buf)
                self.records += r
                self.texts += t
                self.last_data = time.time()


def stream(ser, packets, label_timeout=15):
    """Send every packet to ESP32 A, never more than MAX_INFLIGHT unacked."""
    n = len(packets)
    sent = acked = 0
    last_progress = time.time()
    t0 = time.time()
    while sent < n:
        waiting = ser.in_waiting
        if waiting:
            got = ser.read(waiting).count(b"A")
            if got:
                acked += got
                last_progress = time.time()
        if sent - acked * ACK_EVERY >= MAX_INFLIGHT:
            if time.time() - last_progress > label_timeout:
                sys.exit("Sender stalled. Is ESP32 B powered, wired to the same bus "
                         "and showing READY? Without a receiver nobody ACKs frames.")
            time.sleep(0.0005)
            continue
        k = min(ACK_EVERY, n - sent)
        ser.write(b"".join(packets[sent:sent + k]))
        sent += k
        if sent % 5000 < k:
            rate = sent / (time.time() - t0)
            print("  sent %d / %d frames (%.0f frames/s)" % (sent, n, rate))
    return time.time() - t0


def run_hardware(args):
    import joblib
    from forest import hard_vote_predict

    bundle = joblib.load(args.model)
    model, names = bundle["model"], bundle["class_names"]
    X, y = bundle["X_test"], bundle["y_test"]
    if args.n:
        X, y = X[:args.n], y[:args.n]
    print("Test frames: %d (held out from training)" % len(X))
    y_python = model.classes_[hard_vote_predict(model, X)]

    rx = open_board(args.receiver, args.baud, "Receiver (ESP32 B)")
    rx.write(b"Z")
    tx = open_board(args.sender, args.baud, "Sender   (ESP32 A)")
    col = Collector(rx, frames=args.gateway)
    col.start()

    packets = [pack_frame(r) for r in X]
    secs = stream(tx, packets)

    # Let the last frames cross the bus and the receiver, then ask for its count.
    while time.time() - col.last_data < 1.0:
        time.sleep(0.1)
    rx.write(b"Q")
    t0 = time.time()
    while not any(t.startswith("#DONE") for t in col.texts) and time.time() - t0 < 5:
        time.sleep(0.1)
    col.stop = True
    done = next((t for t in col.texts if t.startswith("#DONE")), "no #DONE line")
    print("Sent %d frames in %.1f s. Receiver says: %s" % (len(X), secs, done))
    m = re.search(r"ovf=(\d+)", done)
    if m and int(m.group(1)) > 0:
        print("WARNING: the CAN controller dropped frames, the receiver could not keep up.")

    if args.gateway:
        finish_gateway(X, y, y_python, col.records, list(names))
        return
    classes = np.asarray(model.classes_)
    finish(X[:, 0], y, y_python, col.records, classes, names)


def finish(sent_ids, y_true, y_python, records, classes, names):
    """Align what came back with what was sent, then score."""
    rec_cls = np.array([r[0] for r in records], dtype=np.int64)
    rec_ids = [r[1] for r in records]
    idx, lost = align([int(i) for i in sent_ids], rec_ids)
    keep = np.array([i >= 0 for i in idx], dtype=bool)
    pos = np.array([i for i in idx if i >= 0], dtype=np.int64)
    print("Received %d records, %d sent frames lost on the way, %d unmatched records"
          % (len(records), lost, int((~keep).sum())))
    if len(pos) == 0:
        sys.exit("Nothing to score. Check the receiver wiring.")
    if lost:
        print("Note: lost frames are left out of the scores. A few is a wiring or "
              "timing problem worth fixing before you trust the numbers.")
    present = np.asarray(classes)
    report(np.asarray(y_true)[pos], present[rec_cls[keep]], np.asarray(y_python)[pos],
           list(names))


def finish_gateway(X, y_true, y_python, got, names):
    """Gateway run: a frame that never came out the far side was blocked."""
    sent = [tuple(int(v) for v in row[:10]) for row in X]
    arrived, spurious = align_frames(sent, got)
    print("%d frames came out of the gateway, %d unmatched" % (len(got), spurious))
    if spurious:
        print("Note: unmatched frames mean corruption or reordering on the bus.")
    report_gateway(y_true, arrived, y_python, names)
    print("\nA frame that is missing counts as blocked. Frames lost to wiring "
          "problems look the same, so check the Normal row first: it should "
          "be close to 100% passed.")


# ---- self test, no hardware ---------------------------------------------------

def selftest():
    rng = np.random.RandomState(0)
    n = 5000
    ids = rng.randint(0, 0x800, n)
    X = np.zeros((n, 10), dtype=np.int32)
    X[:, 0], X[:, 1] = ids, 8
    X[:, 2:] = rng.randint(0, 256, (n, 8))
    y = (ids % 5).astype(np.int64)                 # fake label
    py = y.copy()
    py[::50] = (py[::50] + 1) % 5                  # Python model wrong on 2%
    esp = py.copy()                                # ESP32 copy matches Python exactly

    # Wire format round trip, with noise bytes and a text line in the stream.
    raw = bytearray(b"junk\n")
    drop = {10, 11, 900, 4999}                     # frames the bus "lost"
    for i in range(n):
        if i in drop:
            continue
        raw += bytes([SYNC, int(esp[i]), ids[i] >> 8, ids[i] & 0xFF])
        if i == 2000:
            raw += b"#DONE rx=1 ovf=0\n"
    first = bytearray(raw[:1001])                  # split mid record on purpose
    rec1, tx1 = parse_stream(first)
    first += raw[1001:]
    rec2, tx2 = parse_stream(first)
    records, texts = rec1 + rec2, tx1 + tx2
    assert len(records) == n - len(drop), (len(records), n - len(drop))
    assert texts == ["#DONE rx=1 ovf=0"], texts
    assert not first, "tail left over"

    sent_ids = [int(i) for i in X[:, 0]]
    idx, lost = align(sent_ids, [r[1] for r in records])
    assert lost == len(drop), lost
    keep = [i for i in idx if i >= 0]
    assert len(keep) == len(records)
    # A lost frame followed by one with a different ID must not shift labels.
    ok = sum(1 for k, i in enumerate(keep) if ids[i] == records[k][1])
    assert ok == len(records), "alignment drifted"
    assert pack_frame(X[0])[:3] == bytes([ids[0] >> 8, ids[0] & 0xFF, 8])
    assert len(pack_frame(X[0])) == PACKET
    print("selftest passed: parse, align and pack work, %d drops detected" % lost)
    finish(sent_ids, y, py, records, np.arange(5),
           ["Normal", "DoS", "Fuzzy", "Gear_spoof", "RPM_spoof"])


def selftest_gateway():
    rng = np.random.RandomState(1)
    n = 4000
    X = np.zeros((n, 10), dtype=np.int32)
    X[:, 0] = rng.choice([0x316, 0x43F, 0x153], n)
    X[:, 1] = 8
    X[:, 2:] = rng.randint(0, 4, (n, 8))
    y = np.zeros(n, dtype=np.int64)
    y[1000:1300] = 1
    X[1000:1300, 0] = 0                       # a 300 frame flood the gateway blocks
    blocked = y == 1
    blocked[2500] = True                      # one false positive
    got_rows = [tuple(int(v) for v in X[i]) for i in range(n) if not blocked[i]]
    raw = bytearray()
    for fr in got_rows:
        raw += bytes([SYNC_FRAME, fr[0] >> 8, fr[0] & 0xFF, fr[1]] + list(fr[2:]))
    raw += b"#DONE rx=5 ovf=0\n"
    half = bytearray(raw[:5001])
    f1, t1 = parse_frame_stream(half)
    half += raw[5001:]
    f2, t2 = parse_frame_stream(half)
    assert f1 + f2 == got_rows, "frame parse mismatch"
    assert t1 + t2 == ["#DONE rx=5 ovf=0"]
    sent = [tuple(int(v) for v in r) for r in X]
    arrived, spurious = align_frames(sent, got_rows)
    assert spurious == 0 and arrived == [not b for b in blocked.tolist()], "align wrong"
    print("gateway selftest passed: %d of %d frames came out" % (len(got_rows), n))
    report_gateway(y, arrived, (y != 0).astype(int), ["Normal", "DoS"])


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender", help="serial port of ESP32 A, for example COM5")
    ap.add_argument("--receiver", help="serial port of ESP32 B, for example COM6")
    ap.add_argument("--model", default=os.path.join(ROOT, "models", "can_ids_forest.joblib"))
    ap.add_argument("--n", type=int, default=20000,
                    help="how many test frames to send, 0 sends the whole test set")
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--gateway", action="store_true",
                    help="the receiver is the pass-through sketch behind an STM32 gateway: "
                         "score by which frames get through")
    ap.add_argument("--copy-model", action="store_true",
                    help="copy mcu/generated/can_ids_model.h into the receiver sketches and the STM32 gateway, then exit")
    ap.add_argument("--selftest", action="store_true",
                    help="check the scoring code without any hardware and exit")
    args = ap.parse_args()

    if args.selftest:
        selftest()
        selftest_gateway()
        return
    if args.copy_model:
        src = os.path.join(ROOT, "mcu", "generated", "can_ids_model.h")
        if not os.path.exists(src):
            sys.exit("Run python/export_c.py first, %s does not exist" % src)
        for dst in (os.path.join(ROOT, "esp32", "receiver", "can_ids_model.h"),
                    os.path.join(ROOT, "esp32", "receiver_twai", "can_ids_model.h"),
                    os.path.join(ROOT, "mcu", "stm32_gateway", "src", "can_ids_model.h")):
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(src, dst)
            print("Copied model to", dst)
        return
    if not (args.sender and args.receiver):
        ap.error("--sender and --receiver are required")
    run_hardware(args)


if __name__ == "__main__":
    main()
