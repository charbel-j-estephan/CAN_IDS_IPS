"""Send the Car Hacking dataset over a real CAN bus and check it arrives intact.

No model, no training. PC -> ESP32 A (sender_twai) -> CAN bus -> ESP32 B
(passthrough_twai) -> PC. The PC compares what ESP32 B heard with what it sent.

    python python/send_dataset.py --sender COM5 --receiver COM6
    python python/send_dataset.py --sender COM5 --receiver COM6 --file DoS_dataset.csv --n 50000

Flash esp32/sender_twai on ESP32 A and esp32/passthrough_twai on ESP32 B first.
Needs numpy and pyserial (pip install numpy pyserial).
"""
import argparse
import os
import sys
import time

import numpy as np

import esp32_eval as E
from dataset import ATTACK_FILES, NORMAL_FILE, CLASS_NAMES, parse_attack_csv, parse_normal_txt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load_frames(data_dir, name, n):
    path = os.path.join(data_dir, name)
    if not os.path.exists(path):
        sys.exit("%s not found. Put the dataset files in %s" % (name, data_dir))
    if name == NORMAL_FILE:
        X, y = parse_normal_txt(path, n)
    else:
        X, y = parse_attack_csv(path, ATTACK_FILES[name], n)
    return np.asarray(X, dtype=np.int32), np.asarray(y, dtype=np.int32)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sender", required=True, help="serial port of ESP32 A, for example COM5")
    ap.add_argument("--receiver", required=True, help="serial port of ESP32 B, for example COM6")
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--file", default=NORMAL_FILE,
                    help="which dataset file to send (default %s)" % NORMAL_FILE)
    ap.add_argument("--n", type=int, default=20000, help="how many frames to send")
    ap.add_argument("--baud", type=int, default=921600)
    args = ap.parse_args()

    X, y = load_frames(args.data, args.file, args.n)
    print("Loaded %d frames from %s" % (len(X), args.file))

    rx = E.open_board(args.receiver, args.baud, "Receiver (ESP32 B)")
    rx.write(b"Z")
    tx = E.open_board(args.sender, args.baud, "Sender   (ESP32 A)")
    col = E.Collector(rx, frames=True)
    col.start()

    secs = E.stream(tx, [E.pack_frame(r) for r in X])

    while time.time() - col.last_data < 1.0:
        time.sleep(0.1)
    rx.write(b"Q")
    t0 = time.time()
    while not any(t.startswith("#DONE") for t in col.texts) and time.time() - t0 < 5:
        time.sleep(0.1)
    col.stop = True
    done = next((t for t in col.texts if t.startswith("#DONE")), "no #DONE line")

    sent = [tuple(int(v) for v in row[:10]) for row in X]
    arrived, spurious = E.align_frames(sent, col.records)
    got = sum(arrived)
    print("\nSent      %d frames in %.1f s (%.0f frames/s)" % (len(sent), secs, len(sent) / secs))
    print("Received  %d frames, receiver says: %s" % (len(col.records), done))
    print("Arrived intact (same ID, length and data): %d of %d (%.2f%%)"
          % (got, len(sent), 100.0 * got / len(sent)))
    print("Lost: %d    Received but not matching anything sent: %d"
          % (len(sent) - got, spurious))
    if len(col.records) == 0:
        print("\nNothing arrived. Check the receiver wiring and that both boards print READY.")
    elif got == len(sent):
        print("\nEvery frame crossed the bus unchanged.")
    elif spurious:
        print("\nSome frames arrived altered. Check the CANH/CANL wiring and termination.")
    else:
        print("\nSome frames never arrived. Try a larger GAP_US in the sender, or check the wiring.")


if __name__ == "__main__":
    main()
