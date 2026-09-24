"""Train the random forest IDS on the Car Hacking Dataset.

Example:
    python python/train.py --data data/car_hacking --trees 100 --max-depth 10

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
    ap.add_argument("--test-size", type=float, default=0.2)
    ap.add_argument("--out", default="models/can_ids_forest.joblib")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    X, y = load(args.data, args.limit_per_file or None)
    X_tr, X_te, y_tr, y_te = train_test_split(
        X, y, test_size=args.test_size, stratify=y, random_state=args.seed)

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
    nodes = sum(t.tree_.node_count for t in model.estimators_)
    depth = max(t.tree_.max_depth for t in model.estimators_)
    print("Forest size: %d nodes total, deepest tree %d levels" % (nodes, depth))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    joblib.dump({
        "model": model,
        "class_names": names,
        "features": FEATURES,
        "X_test": X_te,
        "y_test": y_te,
    }, args.out)
    print("Saved", args.out)


if __name__ == "__main__":
    main()
