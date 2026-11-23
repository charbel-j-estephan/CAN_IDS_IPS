import os
import sys, json, joblib, numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
from dataset import load
from forest import hard_vote_predict, known_id_rule
X, y, src = load(os.environ.get("CAR_HACKING_DIR", "data"), with_source=True)
X[:, 7:] = 0
b = joblib.load("rf_p5.joblib"); m = b["model"]
files = ["DoS","Fuzzy","gear","RPM","normal"]
res = {}
for s in np.unique(src):
    idx = np.flatnonzero(src == s)
    p = known_id_rule(m.classes_[hard_vote_predict(m, X[idx])], X[idx, 0], b["known_ids"])
    fp = np.flatnonzero((p != 0) & (y[idx] == 0))
    a = np.flatnonzero(y[idx] != 0); cut = a[int(len(a)*0.8)] if len(a) else int(len(idx)*0.8)
    res[files[s]] = {"rows": len(idx), "cut": int(cut), "fp_total": len(fp), "fp_after_cut": int(np.sum(fp >= cut)),
                     "fp_first_rows": fp[:5].tolist(), "fp_ids": sorted({hex(int(i)) for i in X[idx][fp, 0]})}
    if len(fp):
        res[files[s]]["fp_row_range"] = [int(fp.min()), int(fp.max())]
print(json.dumps(res, indent=1)); json.dump(res, open("fp_rows.json","w"), indent=1)
