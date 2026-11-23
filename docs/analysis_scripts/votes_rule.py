import os
import sys, json
import joblib, numpy as np
from collections import Counter
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
from dataset import load
X0, y, src = load(os.environ.get("CAR_HACKING_DIR", "data"), with_source=True)
test = np.zeros(len(y), bool)
for s in np.unique(src):
    idx = np.flatnonzero(src == s); a = np.flatnonzero(y[idx] != 0)
    cut = a[int(len(a)*0.8)] if len(a) else int(len(idx)*0.8); test[idx[cut:]] = True
yt = y[test]; N = ["Normal","DoS","Fuzzy","Gear","RPM"]; out = {}
for p in (5, 6):
    X = X0[test].copy(); X[:, 2+p:] = 0
    b = joblib.load(f"rf_p{p}.joblib"); m = b["model"]; known = b["known_ids"]
    Xf = X.astype(np.float32); v = np.zeros((len(X), 5), np.int8); r = np.arange(len(X))
    for t in m.estimators_: v[r, np.argmax(t.predict_proba(Xf), 1)] += 1
    cls = v.argmax(1); top = v.max(1); unknown = ~np.isin(X[:, 0], known)
    res = {"fp_by_id": {hex(k): c for k, c in Counter(int(i) for i in X[((cls != 0) | unknown) & (yt == 0), 0]).most_common()}}
    for k in (3, 4, 5):
        d = unknown | ((cls != 0) & (top >= k))
        e = {"normal_destroyed": int(np.sum(d & (yt == 0)))}
        for c in range(1, 5):
            mm = yt == c; e[N[c]+"_destroyed_pct"] = round(100*float(np.mean(d[mm])), 3); e[N[c]+"_passed"] = int(np.sum(mm & ~d))
        res[f"votes_at_least_{k}"] = e
    out[f"p{p}"] = res
json.dump(out, open("votes_rule.json", "w"), indent=1); print(json.dumps(out, indent=1))
