"""Train the random forest IDS on the Car Hacking Dataset.

Example:
    python python/train.py --data data/car_hacking --trees 100 --max-depth 10
    python python/train.py --data data/car_hacking --trees 5 --max-depth 9 --payload-bytes 5

--split time (default) cuts every file at one moment: the model trains on
what came before and is scored on what came after, traffic it never saw. In
an attack file the cut is where --test-size of its attack frames are still
to come (the attacks fill only the first ~80% of each recording, 300 episodes
each, so cutting at 80% of the frames would leave almost none to test on);
in the normal file it is at 1 - --test-size of its frames. --split random
scores the model on frames that are often exact copies of training frames
(a DoS flood is one frame repeated), so it looks better than it is.

Lower --trees or --max-depth when the FPGA misses its latency budget or does
not fit. Every run saves the model plus a held out test set that the Verilog
and C exporters use to build test vectors.
"""
import argparse
import os
import time

import joblib
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report
from sklearn.metrics import confusion_matrix
from sklearn.model_selection import train_test_split

from dataset import CLASS_NAMES, FEATURES, load
from forest import hard_vote_predict


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", required=True, help="folder with the dataset files")
    ap.add_argument("--trees", type=int, default=100, help="n_estimators")
    ap.add_argument("--max-depth", type=int, default=10,
                    help="max tree depth, 0 means unlimited (huge hardware)")
    ap.add_argument("--limit-per-file", type=int, default=0,
                    help="read at most this many frames per file, 0 reads all")
    ap.add_argument("--payload-bytes", type=int, default=8, choices=range(9),
                    metavar="N",
                    help="use only the first N data bytes (0 to 8). Fewer bytes "
                         "lets the detector decide earlier in the frame, see "
                         "python/latency.py")
    ap.add_argument("--test-size", type=float, default=0.2)
    ap.add_argument("--split", choices=["time", "random"], default="time",
                    help="time: test on the last part of every file (default); "
                         "random: shuffled, stratified")
    ap.add_argument("--out", default="models/can_ids_forest.joblib")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    X, y, src = load(args.data, args.limit_per_file or None, with_source=True)
    if args.payload_bytes < 8:
        # Zero the unused bytes so no tree can split on them. The hardware
        # and C ports stay the same, the trees just never read those bytes.
        X[:, 2 + args.payload_bytes:] = 0
        print("Using CAN ID, DLC and the first %d data bytes" % args.payload_bytes)
    if args.split == "time":
        test = np.zeros(len(y), dtype=bool)
        for s in np.unique(src):
            idx = np.flatnonzero(src == s)           # this file's frames, in recorded order
            attack = np.flatnonzero(y[idx] != 0)
            if len(attack):                          # cut where test_size of the attacks remain
                cut = attack[int(len(attack) * (1 - args.test_size))]
            else:
                cut = int(len(idx) * (1 - args.test_size))
            test[idx[cut:]] = True
        X_tr, X_te, y_tr, y_te = X[~test], X[test], y[~test], y[test]
    else:
        X_tr, X_te, y_tr, y_te = train_test_split(
            X, y, test_size=args.test_size, stratify=y, random_state=args.seed)
    print("Split: %s, %d training frames, %d test frames" % (args.split, len(y_tr), len(y_te)))
    for c, name in enumerate(CLASS_NAMES):
        print("  %-10s train %9d  test %9d" % (name, (y_tr == c).sum(), (y_te == c).sum()))

    model = RandomForestClassifier(
        n_estimators=args.trees,
        max_depth=args.max_depth or None,
        n_jobs=-1,
        random_state=args.seed,
    )
    t0 = time.time()
    model.fit(X_tr, y_tr)
    print("Trained %d trees in %.1f s" % (args.trees, time.time() - t0))

    names = [CLASS_NAMES[c] for c in model.classes_]
    soft = model.predict(X_te)
    hard = model.classes_[hard_vote_predict(model, X_te)]
    print("Accuracy, scikit learn soft vote: %.5f" % accuracy_score(y_te, soft))
    print("Accuracy, hardware hard vote:     %.5f" % accuracy_score(y_te, hard))
    print(classification_report(y_te, hard, labels=model.classes_,
                                target_names=names, digits=4))
    print("Confusion matrix (rows: true class, columns: predicted), hard vote:")
    cm = confusion_matrix(y_te, hard, labels=model.classes_)
    print("  %-10s" % "" + "".join("%11s" % n for n in names))
    for name, row in zip(names, cm):
        print("  %-10s" % name + "".join("%11d" % v for v in row))
    nodes = sum(t.tree_.node_count for t in model.estimators_)
    depth = max(t.tree_.max_depth for t in model.estimators_)
    print("Forest size: %d nodes total, deepest tree %d levels" % (nodes, depth))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    joblib.dump({
        "model": model,
        "class_names": names,
        "features": FEATURES,
        "payload_bytes": args.payload_bytes,
        "split": args.split,
        "X_test": X_te,
        "y_test": y_te,
    }, args.out)
    print("Saved", args.out)


if __name__ == "__main__":
    main()
