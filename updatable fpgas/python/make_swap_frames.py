"""Bus traffic for the mid-traffic swap test, built so the test can tell the two
models apart: half the frames are ones where the 5 byte and the 8 byte model
give different answers. Without them a frame answered by the wrong model would
look correct.

    py -3.13 make_swap_frames.py --frames 120 --out build/sim_swap

Writes bus_bits.mem, frames.vh and sent.csv in the format python/make_sim_frames.py
uses, so tb/core_frames_tb.v plays it unchanged.
"""
import argparse
import csv
import os
import random
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import paths  # noqa: E402

sys.path.insert(0, os.path.join(paths.REPO, "python"))
from forest import hard_vote_predict, known_id_rule  # noqa: E402
from make_can_vectors import frame_on_bus  # noqa: E402
from make_sim_frames import FILES, read_pools  # noqa: E402

SCAN_ROWS = 400000


def verdicts(model_name, frames):
    import joblib
    b = joblib.load(paths.find_shared("models", model_name))
    nb = int(b["payload_bytes"])
    X = []
    for cid, dlc, data in frames:
        d = (list(data[:dlc]) + [0] * 8)[:8]
        X.append([cid, min(dlc, 8)] + d[:nb] + [0] * (8 - nb))
    X = np.array(X, dtype=np.float32)
    forest = hard_vote_predict(b["model"], X)
    return known_id_rule(forest, X[:, 0].astype(int), b["known_ids"])


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--frames", type=int, default=120)
    ap.add_argument("--out", required=True)
    ap.add_argument("--seed", type=int, default=7)
    args = ap.parse_args()
    rng = random.Random(args.seed)

    data_dir = paths.find_shared("data", "car_hacking")
    seen = {}
    for name in FILES:
        normal, attack = read_pools(os.path.join(data_dir, name), SCAN_ROWS)
        for cid, dlc, data in normal + attack:
            seen[(cid, dlc, tuple(data))] = 1
    pool = [(c, d, list(x)) for c, d, x in seen]
    v5 = verdicts("rf5_d9_p5_time.joblib", pool)
    v8 = verdicts("rf5_d9_p8_time.joblib", pool)
    differ = [pool[i] for i in np.flatnonzero(v5 != v8)]
    same = [pool[i] for i in np.flatnonzero(v5 == v8)]
    print("Distinct frames %d, the two models disagree on %d" % (len(pool), len(differ)))
    if len(differ) < args.frames // 2:
        raise SystemExit("Not enough frames where the models disagree")

    # Disagreeing frames all through the run, so both sides of the swap have some.
    n_diff = args.frames // 2
    picked = rng.sample(differ, n_diff) + rng.sample(same, args.frames - n_diff)
    rng.shuffle(picked)

    stream = [1] * 20
    starts = []
    for cid, dlc, data in picked:
        starts.append(len(stream))
        stream += frame_on_bus(cid, dlc, data)
        stream += [1] * rng.randint(0, 40)

    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "bus_bits.mem"), "w") as f:
        f.write("\n".join(str(b) for b in stream) + "\n")
    with open(os.path.join(args.out, "frames.vh"), "w") as f:
        f.write("localparam N_BITS = %d;\nlocalparam N_FRAMES = %d;\n" % (len(stream), len(picked)))
    with open(os.path.join(args.out, "sent.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["n", "start_bit", "id", "dlc", "data"])
        for k, ((cid, dlc, data), s) in enumerate(zip(picked, starts)):
            w.writerow([k, s, "%03X" % cid, dlc, " ".join("%02X" % b for b in data[:dlc])])
    print("Wrote %d frames (%d where the models disagree), %d bus bits" % (len(picked), n_diff, len(stream)))


if __name__ == "__main__":
    main()
