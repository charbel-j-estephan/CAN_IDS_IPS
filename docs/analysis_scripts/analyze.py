"""Extra analyses for the paper, on the unseen part of the time split.

1. Ablation: allow list alone, forest alone, both.
2. Short frames: DLC histogram and the real deadline for frames with DLC < 8.
3. False positives: what happens to the real sender when a genuine frame is
   destroyed and retransmitted, and a burst rule that avoids it.
"""
import json
import os
import re
import sys
from collections import Counter, defaultdict

import joblib
import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from dataset import ATTACK_FILES, NORMAL_FILE, load  # noqa: E402
from forest import known_id_rule  # noqa: E402

DATA = os.environ.get("CAR_HACKING_DIR", "data")
NAMES = ["Normal", "DoS", "Fuzzy", "Gear", "RPM"]
out = {}


def timestamps():
    """Timestamps in the same order and with the same filtering as dataset.load."""
    ts = []
    for name in ATTACK_FILES:
        with open(f"{DATA}/{name}") as f:
            for line in f:
                parts = line.strip().split(",")
                if len(parts) < 4:
                    continue
                try:
                    int(parts[1], 16)
                    dlc = int(parts[2])
                    [int(b, 16) for b in parts[3:3 + dlc]]
                except ValueError:
                    continue
                ts.append(float(parts[0]))
    rx = re.compile(r"Timestamp:\s*([\d.]+).*ID:\s*([0-9a-fA-F]+)\s+\S+\s+DLC:\s*(\d+)\s*((?:[0-9a-fA-F]{2}\s*)*)$")
    with open(f"{DATA}/{NORMAL_FILE}") as f:
        for line in f:
            m = rx.search(line.strip())
            if m:
                ts.append(float(m.group(1)))
    return np.asarray(ts)


def votes(model, X):
    X = np.asarray(X, dtype=np.float32)
    v = np.zeros((len(X), len(model.classes_)), dtype=np.int8)
    rows = np.arange(len(X))
    for t in model.estimators_:
        v[rows, np.argmax(t.predict_proba(X), axis=1)] += 1
    return v


X, y, src = load(DATA, with_source=True)
ts = timestamps()
assert len(ts) == len(y), (len(ts), len(y))
X[:, 2 + 5:] = 0
Xfull, _ = None, None

test = np.zeros(len(y), dtype=bool)
for s in np.unique(src):
    idx = np.flatnonzero(src == s)
    attack = np.flatnonzero(y[idx] != 0)
    cut = attack[int(len(attack) * 0.8)] if len(attack) else int(len(idx) * 0.8)
    test[idx[cut:]] = True

bundle = joblib.load("rf_p5.joblib")
model, known = bundle["model"], bundle["known_ids"]
Xt, yt, tt, st = X[test], y[test], ts[test], src[test]
print("test frames", len(yt))

v = votes(model, Xt)
forest = np.argmax(v, axis=1)
both = known_id_rule(forest, Xt[:, 0], known)
allow = np.where(np.isin(Xt[:, 0], known), 0, 2)

# 1. Ablation
abl = {}
for label, p in [("allow_list_only", allow), ("forest_only", forest), ("allow_list_plus_forest", both)]:
    r = {"accuracy_binary_pct": round(100 * np.mean((p != 0) == (yt != 0)), 4),
         "normal_flagged": int(np.sum((p != 0) & (yt == 0)))}
    for c in range(1, 5):
        m = yt == c
        r[NAMES[c] + "_flagged_pct"] = round(100 * np.mean(p[m] != 0), 3)
    abl[label] = r
out["ablation"] = abl

# 2. DLC
dlc = Xt[:, 1]
d = {}
for k, cnt in sorted(Counter(dlc.tolist()).items()):
    ids = sorted({int(i) for i in Xt[dlc == k, 0]})
    d[int(k)] = {"frames": int(cnt), "ids": [hex(i) for i in ids][:10],
                 "attack_frames": int(np.sum((dlc == k) & (yt != 0)))}
out["dlc"] = d

# 3. False positives
fp = (both != 0) & (yt == 0)
fp_ids = Counter(int(i) for i in Xt[fp, 0])
fp_votes = Counter(int(m) for m in v[fp].max(axis=1))
att = yt != 0
att_votes = Counter(int(m) for m in v[att & (both != 0) & (forest != 0)].max(axis=1))
out["false_positives"] = {
    "count": int(fp.sum()),
    "by_id": {hex(k): c for k, c in fp_ids.most_common()},
    "winning_votes_on_false_flags": dict(sorted(fp_votes.items())),
    "winning_votes_on_true_attack_flags": dict(sorted(att_votes.items())),
}

# Burst rule: destroy a frame only when the same ID has had at least M attack
# verdicts within the last W seconds (counting this one). Once it destroys,
# the retransmissions of that same frame are destroyed too (they are part of
# the same event). Evaluated per recording, in time order.
def burst_rule(M, W):
    destroy = np.zeros(len(yt), dtype=bool)
    flagged = both != 0
    for s in np.unique(st):
        idx = np.flatnonzero(st == s)
        idx = idx[np.argsort(tt[idx], kind="stable")]
        recent = defaultdict(list)
        for i in idx:
            if not flagged[i]:
                continue
            cid = int(Xt[i, 0])
            q = recent[cid]
            q.append(tt[i])
            while q and q[0] < tt[i] - W:
                q.pop(0)
            if len(q) >= M:
                destroy[i] = True
    r = {"normal_destroyed": int(np.sum(destroy & (yt == 0)))}
    for c in range(1, 5):
        m = yt == c
        r[NAMES[c] + "_destroyed_pct"] = round(100 * np.mean(destroy[m]), 3)
        r[NAMES[c] + "_passed"] = int(np.sum(m & ~destroy))
    return r


out["burst_rule"] = {}
for M, W in [(1, 0.1), (2, 0.1), (3, 0.1), (2, 0.05), (3, 0.05), (5, 0.1)]:
    out["burst_rule"]["M%d_W%dms" % (M, int(W * 1000))] = burst_rule(M, W)
    print(M, W, out["burst_rule"]["M%d_W%dms" % (M, int(W * 1000))])

# Gaps between consecutive false flags on the same ID (bus off needs repeats)
gaps = []
for cid in fp_ids:
    t = np.sort(tt[fp & (Xt[:, 0] == cid)])
    gaps += np.diff(t).tolist()
gaps = np.asarray(gaps)
out["fp_gap_seconds"] = {"min": float(gaps.min()) if len(gaps) else None,
                         "median": float(np.median(gaps)) if len(gaps) else None,
                         "under_10ms": int(np.sum(gaps < 0.01))}

json.dump(out, open("analysis.json", "w"), indent=1)
print(json.dumps(out, indent=1))
