"""Convert the trained forest to C for the STM32 with emlearn.

Example:
    python python/export_c.py --model models/can_ids_forest.joblib

Writes mcu/generated/can_ids_model.h. It is plain C with no library
dependency. Call it like this:

    float features[10] = {can_id, dlc, d0, d1, d2, d3, d4, d5, d6, d7};
    int32_t cls = can_ids_predict(features, 10);

Two details keep the C model identical to scikit learn:
  1. scikit learn splits with x <= t but emlearn writes x < t. When t is a
     whole number (for example 130.0 because no training frame had 130), a
     frame with exactly 130 would take different branches. Every threshold is
     moved to floor(t) + 0.5 first. For integer features that split is the
     same under both < and <=.
  2. Float thresholds, not emlearn's int16 mode, because int16 rounds 157.5
     down to 157. The STM32F407 has a hardware FPU, so float compares are cheap.

emlearn counts one vote per tree with ties to the lowest class, the same rule
as the FPGA module, so both return the same class for every frame.
"""
import argparse
import copy
import math
import os

import emlearn
import joblib


def integer_safe_copy(model):
    """Copy the forest with every threshold moved to floor(t) + 0.5."""
    model = copy.deepcopy(model)
    for est in model.estimators_:
        thr = est.tree_.threshold  # a view into the tree, edits stick
        split = est.tree_.feature >= 0
        thr[split] = [math.floor(t) + 0.5 for t in thr[split]]
    return model


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--model", default="models/can_ids_forest.joblib")
    ap.add_argument("--out", default="mcu/generated/can_ids_model.h")
    args = ap.parse_args()

    bundle = joblib.load(args.model)
    model = bundle["model"]
    cmodel = emlearn.convert(integer_safe_copy(model), method="inline",
                             dtype="float")
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    cmodel.save(file=args.out, name="can_ids")
    print("Wrote %s (%.1f KB). Classes: %s" % (
        args.out, os.path.getsize(args.out) / 1024,
        ", ".join("%d=%s" % (i, n) for i, n in enumerate(bundle["class_names"]))))


if __name__ == "__main__":
    main()
