"""Proves the mid-traffic model swap: no frame lost, no frame mixed.

    py -3.13 check_swap.py <run folder with sent.csv and got_swap.csv>

Every frame on the bus must get exactly one verdict. The verdicts must form one
run of the old bank then one run of the new bank, and each must equal what
Python gives for the model of its bank, with that model's data bytes.
"""
import csv
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import paths  # noqa: E402

sys.path.insert(0, os.path.join(paths.REPO, "python"))
from dataset import CLASS_NAMES  # noqa: E402
from forest import hard_vote_predict, known_id_rule  # noqa: E402


def predict(model_path, frames):
    import joblib
    b = joblib.load(paths.find_shared("models", model_path))
    nb = int(b["payload_bytes"])
    X = []
    for cid, dlc, data in frames:
        d = (list(data[:dlc]) + [0] * 8)[:8]
        X.append([cid, min(dlc, 8)] + d[:nb] + [0] * (8 - nb))
    X = np.array(X, dtype=np.float32)
    forest = hard_vote_predict(b["model"], X)
    return known_id_rule(forest, X[:, 0].astype(int), b["known_ids"]), nb


def main():
    run = sys.argv[1]
    with open(os.path.join(run, "sent.csv")) as f:
        sent = list(csv.DictReader(f))
    frames = [(int(s["id"], 16), int(s["dlc"]), [int(x, 16) for x in s["data"].split()]) for s in sent]

    finals, banks, feats = [], [], []
    with open(os.path.join(run, "got_swap.csv")) as f:
        for e in csv.DictReader(f):
            if e["event"] == "FEAT":
                feats.append(int(e["data"], 16))
            if e["event"] == "FINAL":
                finals.append((int(e["id"], 16), int(e["class"])))
            if e["event"] == "BANK":
                banks.append(int(e["class"]))

    print("Frames sent %d, verdicts %d" % (len(frames), len(finals)))
    fails = 0
    if len(finals) != len(frames):
        print("FAIL: frames were lost or doubled")
        fails += 1

    old_bank = banks[0]
    change = next((i for i, b in enumerate(banks) if b != old_bank), None)
    if change is None:
        raise SystemExit("FAIL: the bank never changed, the swap did not happen")
    if any(b == old_bank for b in banks[change:]):
        print("FAIL: the bank changed back")
        fails += 1
    print("Swap at frame %d of %d (bank %d -> %d)" % (change, len(banks), old_bank, banks[change]))

    p5, nb5 = predict("rf5_d9_p5_time.joblib", frames)
    p8, nb8 = predict("rf5_d9_p8_time.joblib", frames)
    print("Old model reads %d data bytes, new model %d" % (nb5, nb8))
    differ = 0
    bad = 0
    for i, ((cid, got), (fid, _, _)) in enumerate(zip(finals, frames)):
        if cid != fid:
            print("frame %d: verdict for id %03X, sent %03X" % (i, cid, fid))
            bad += 1
            continue
        want = (p5 if i < change else p8)[i]
        if p5[i] != p8[i]:
            differ += 1
        if got != want:
            print("frame %d id %03X: got %s, want %s (bank %d)"
                  % (i, cid, CLASS_NAMES[got], CLASS_NAMES[want], banks[i]))
            bad += 1
    before = int((p5[:change] != p8[:change]).sum())
    after = int((p5[change:] != p8[change:]).sum())
    print("Frames where the two models disagree: %d (%d before the swap, %d after)" % (differ, before, after))
    if before == 0 or after == 0:
        print("FAIL: this traffic cannot tell the two models apart on one side of the swap")
        fails += 1

    # The runtime byte count: the decoder must hand over exactly the new model's
    # bytes right after the swap, and the old model's before it.
    bytes_bad = 0
    for i, (cid, dlc, data) in enumerate(frames):
        nb = nb5 if i < change else nb8
        want = (list(data[:min(dlc, nb)]) + [0] * 8)[:8]
        val = 0
        for b in want:
            val = (val << 8) | b
        if feats[i] != val:
            bytes_bad += 1
    print("Frames whose features held the wrong number of data bytes: %d" % bytes_bad)
    if len(feats) != len(frames) or bytes_bad:
        print("FAIL: the runtime byte count did not follow the bank")
        fails += 1
    if bad:
        print("FAIL: %d verdicts did not match their bank's model" % bad)
        fails += 1
    if fails:
        raise SystemExit(1)
    print("PASS: every frame got one verdict, each from exactly one whole model")


if __name__ == "__main__":
    main()
