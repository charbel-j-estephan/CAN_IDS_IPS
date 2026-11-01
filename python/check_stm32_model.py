"""Check that the STM32 IDS node decided exactly what Python decides.

    python python/check_stm32_model.py --run "<bench>/runs/<folder>" --model models/rf5_d9_p5_time.joblib

Reads the run's received.csv (frames the STM32 decoded, with its class), runs
the same forest (hard vote) and the unknown ID rule on the same features, and
reports how many frames agree. Anything under 100% is a bug in the firmware's
features or model, not in the model itself: the C forest from export_c.py
takes the same branches as scikit learn on integer features.

Needs scikit-learn (the repo's .venv), not the bench's Python.
"""
import argparse
import collections
import csv
import os
import sys

import joblib
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dataset import CLASS_NAMES                       # noqa: E402
from forest import hard_vote_predict, known_id_rule   # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--run", required=True, help="run folder with received.csv")
    ap.add_argument("--model", default="models/rf5_d9_p5_time.joblib")
    args = ap.parse_args()

    bundle = joblib.load(args.model)
    model, nb = bundle["model"], bundle.get("payload_bytes", 8)
    X, stm = [], []
    with open(os.path.join(args.run, "received.csv")) as f:
        for r in csv.DictReader(f):
            if not r.get("ids_cls"):
                continue
            cls = int(r["ids_cls"])
            if cls >= len(CLASS_NAMES):          # late or model off: nothing to compare
                continue
            dlc = int(r["dlc"])
            data = [int(b, 16) for b in r["data"].split()][:8]
            data += [0] * (8 - len(data))
            data[nb:] = [0] * (8 - nb)           # the model reads the first nb bytes only
            X.append([int(r["can_id"], 16), min(dlc, 8)] + data)
            stm.append(cls)
    if not X:
        sys.exit("No STM32 decisions in %s (an ESP32 receiver run, or the model was off)" % args.run)
    X, stm = np.array(X), np.array(stm)
    py = model.classes_[hard_vote_predict(model, X)]
    if "known_ids" in bundle:
        py = known_id_rule(py, X[:, 0], bundle["known_ids"])
    same = py == stm
    print("Frames decided by the STM32: %d" % len(stm))
    print("Same class as Python: %d (%.4f%%)" % (same.sum(), 100.0 * same.mean()))
    if not same.all():
        diff = collections.Counter((CLASS_NAMES[a], CLASS_NAMES[b]) for a, b in zip(py[~same], stm[~same]))
        print("Disagreements (Python, STM32): %s" % ", ".join("%s->%s %d" % (a, b, n)
                                                         for (a, b), n in diff.most_common(10)))
        for row, a, b in list(zip(X[~same], py[~same], stm[~same]))[:5]:
            print("  ID %03X DLC %d data %s: Python %s, STM32 %s"
                  % (row[0], row[1], " ".join("%02X" % v for v in row[2:2 + nb]),
                     CLASS_NAMES[a], CLASS_NAMES[b]))
        sys.exit(1)


if __name__ == "__main__":
    main()
