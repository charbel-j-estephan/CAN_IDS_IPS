"""Real dataset frames for the FPGA simulation, plus a readable table of what
was sent and what the forest should answer.

    py -3.13 python/make_sim_frames.py                    200 frames, half attacks
    py -3.13 python/make_sim_frames.py --frames 1000
    py -3.13 python/make_sim_frames.py --files DoS_dataset.csv --frames 50
    py -3.13 python/make_sim_frames.py --model models/rf5_d9_p6_time.joblib   6-byte model

Writes build/sim/:
  bus_bits.mem   one bus bit per line (0 dominant, 1 recessive), as RXD shows it,
                 for hdl/frames_tb.v
  frames.vh      N_BITS and N_FRAMES for the testbench
  known_ids.mem  2048 lines, 1 for each ID in the model's known list (hdl/known_id_tb.v)
  sent.csv       one row per frame: where it starts on the bus (bit index), the
                 dataset file and label, ID, DLC, data in hex and as characters,
                 the values the forest reads (ID, DLC, the model's data bytes), and the class Python's forest gives
                 (forest alone, then with the unknown ID rule, as the STM32)

Every frame is a good standard data frame, built bit for bit as a CAN
controller sends it (make_can_vectors.frame_on_bus), with a random idle gap
after it. The stream is decoded again by make_can_vectors.decode before
anything is written. Then compare the FPGA's log with python/compare_sim.py.

Three kinds of answer appear, and only one pair must agree:
  label    the dataset's ground truth (R normal, T attack of that file)
  forest   what the trained forest says: the FPGA must give exactly this
  final    forest + unknown ID rule (Normal on an ID never seen -> Fuzzy)
A forest answer that differs from the label is the model's miss, not a bug.
"""
import argparse
import csv
import os
import random

import numpy as np

from dataset import CLASS_NAMES
from forest import hard_vote_predict, known_id_rule
from make_can_vectors import GOOD, decode, frame_on_bus

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILES = ["DoS_dataset.csv", "Fuzzy_dataset.csv", "gear_dataset.csv", "RPM_dataset.csv"]
ATTACK = {"DoS_dataset.csv": 1, "Fuzzy_dataset.csv": 2, "gear_dataset.csv": 3, "RPM_dataset.csv": 4}
FEATURE_BYTES = 5            # data bytes the model reads: set from the model file in main()
SCAN_ROWS = 200000           # rows read per file to pick frames from


def read_pools(path, rows):
    """Normal (R) and attack (T) frames from one dataset file."""
    normal, attack = [], []
    with open(path) as f:
        for k, line in enumerate(f):
            if k >= rows:
                break
            p = line.strip().split(",")
            try:
                cid, dlc = int(p[1], 16), int(p[2])
                data = [int(b, 16) for b in p[3:3 + dlc]]
            except (ValueError, IndexError):
                continue
            flag = p[3 + dlc].strip() if len(p) > 3 + dlc else "R"
            frame = (cid & 0x7FF, min(dlc, 8), data[:8])
            (attack if flag == "T" else normal).append(frame)
    return normal, attack


def features(cid, dlc, data):
    """What the forest sees at feat_ready: ID, DLC (at most 8), the first
    FEATURE_BYTES data bytes (0 past the DLC), the rest still 0 because they are
    not on the bus yet."""
    d = (list(data[:dlc]) + [0] * 8)[:8]
    return [cid, min(dlc, 8)] + d[:FEATURE_BYTES] + [0] * (8 - FEATURE_BYTES)


def ascii_of(data):
    return "".join(chr(b) if 32 <= b < 127 else "." for b in data)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--files", nargs="+", default=FILES, help="dataset files to take frames from")
    ap.add_argument("--frames", type=int, default=200, help="frames in total")
    ap.add_argument("--attack-share", type=float, default=0.5,
                    help="share of attack (T) frames, the rest normal (R)")
    ap.add_argument("--model", default=os.path.join(ROOT, "models", "rf5_d9_p5_time.joblib"),
                    help="the model the FPGA's rf_ids.v was generated from")
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "sim"))
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    rng = random.Random(args.seed)

    # Pick frames: from each file the same number, attack and normal mixed.
    per_file = max(1, args.frames // len(args.files))
    picked = []
    for name in args.files:
        path = os.path.join(args.data, name)
        if not os.path.exists(path):
            raise SystemExit("Missing dataset file: %s" % path)
        normal, attack = read_pools(path, SCAN_ROWS)
        n_att = min(len(attack), round(per_file * args.attack_share))
        n_norm = min(len(normal), per_file - n_att)
        picked += [(name, "T", f) for f in rng.sample(attack, n_att)]
        picked += [(name, "R", f) for f in rng.sample(normal, n_norm)]
    rng.shuffle(picked)

    # Python's answer for every frame, from the same model as rf_ids.v.
    import joblib
    global FEATURE_BYTES
    bundle = joblib.load(args.model)
    FEATURE_BYTES = int(bundle.get("payload_bytes", 8))
    X = np.array([features(*f) for _, _, f in picked], dtype=np.float32)
    forest = hard_vote_predict(bundle["model"], X)
    final = known_id_rule(forest, X[:, 0].astype(int), bundle.get("known_ids", []))

    # The bus: idle, then every frame with a random gap after it.
    stream = [1] * 20
    starts = []
    for _, _, (cid, dlc, data) in picked:
        starts.append(len(stream))
        stream += frame_on_bus(cid, dlc, data)
        stream += [1] * rng.randint(0, 40)

    got = decode(stream)
    want = [(GOOD, c, d, list(x[:d])) for _, _, (c, d, x) in picked]
    if [(g[0], g[1], g[2], list(g[3])) for g in got] != want:
        raise SystemExit("Internal check failed: the bit stream does not decode back to "
                         "the frames. Nothing written.")

    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "bus_bits.mem"), "w") as f:
        f.write("\n".join(str(b) for b in stream) + "\n")
    known = set(int(i) for i in bundle.get("known_ids", []))
    with open(os.path.join(args.out, "known_ids.mem"), "w") as f:   # for hdl/known_id_tb.v
        f.write("\n".join("1" if i in known else "0" for i in range(2048)) + "\n")
    with open(os.path.join(args.out, "frames.vh"), "w") as f:
        f.write("// Generated by python/make_sim_frames.py\n")
        f.write("localparam N_BITS = %d;\nlocalparam N_FRAMES = %d;\n" % (len(stream), len(picked)))

    rows = []
    for k, ((name, flag, (cid, dlc, data)), s) in enumerate(zip(picked, starts)):
        label = CLASS_NAMES[ATTACK[name]] if flag == "T" else "Normal"
        x = features(cid, dlc, data)
        rows.append({
            "n": k, "start_bit": s, "file": name, "label": label,
            "id": "%03X" % cid, "dlc": dlc,
            "data": " ".join("%02X" % b for b in data[:dlc]), "chars": ascii_of(data[:dlc]),
            "f_id": x[0], "f_dlc": x[1],
            **{"f_d%d" % b: x[2 + b] for b in range(FEATURE_BYTES)},
            "forest": CLASS_NAMES[forest[k]], "final": CLASS_NAMES[final[k]],
        })
    with open(os.path.join(args.out, "sent.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)

    print("Wrote %d frames, %d bus bits (%.1f ms of bus time) to %s"
          % (len(picked), len(stream), len(stream) * 0.002, args.out))
    print("Checked: the bit stream decodes back to exactly these frames.\n")
    print("%-4s %-9s %-10s %-3s %-3s %-23s %-8s %-27s %-10s %s"
          % ("n", "start_bit", "label", "ID", "DLC", "data (hex)", "chars",
             "forest sees (id dlc d0..d%d)" % (FEATURE_BYTES - 1), "forest", "final"))
    for r in rows[:15]:
        feats = " ".join(str(r[c]) for c in r if c.startswith("f_"))
        print("%-4d %-9d %-10s %-3s %-3d %-23s %-8s %-27s %-10s %s"
              % (r["n"], r["start_bit"], r["label"], r["id"], r["dlc"], r["data"], r["chars"],
                 feats, r["forest"], r["final"]))
    if len(rows) > 15:
        print("... %d more in sent.csv" % (len(rows) - 15))


if __name__ == "__main__":
    main()
