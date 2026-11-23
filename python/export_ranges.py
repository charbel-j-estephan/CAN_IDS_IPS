"""Per ID payload ranges for the guarded vote rule (STM32 fw 18).

For each of the 38 known IDs, the min and max of data bytes 0-4 over the
NORMAL frames of the TRAINING split only (time split: first 80 percent of
each capture, the same split as train.py). Test frames are never read.
Writes mcu/stm32_ids/src/can_ids_ranges.h.

Rule in firmware (classify_guarded):
  unknown ID or 5 of 5 trees vote attack     -> destroy (strong)
  3 or 4 trees vote attack AND a byte is out of range -> destroy (weak)
  otherwise                                  -> pass
Weak verdicts on the same frame are capped at 4 in a row (bus off guard).
"""
import sys, os, json
import numpy as np
from dataset import load

P = 5
data = sys.argv[1] if len(sys.argv) > 1 else "data"
X, y, src = load(data, with_source=True)
X[:, 2 + P:] = 0
test = np.zeros(len(y), bool)
for s in np.unique(src):
    idx = np.flatnonzero(src == s); a = np.flatnonzero(y[idx] != 0)
    cut = a[int(len(a) * 0.8)] if len(a) else int(len(idx) * 0.8)
    test[idx[cut:]] = True
train_normal = (~test) & (y == 0)
ids = sorted(int(i) for i in np.unique(X[train_normal, 0]))
lo, hi = [], []
for i in ids:
    d = X[train_normal & (X[:, 0] == i)][:, 2:2 + P]
    lo.append(d.min(0).tolist()); hi.append(d.max(0).tolist())
L = ["/* Per ID payload ranges (bytes 0-4) of the NORMAL frames in the training",
     " * split only. Made by python/export_ranges.py. Used by classify_guarded. */",
     "#ifndef CAN_IDS_RANGES_H", "#define CAN_IDS_RANGES_H", "#include <stdint.h>",
     f"#define CAN_IDS_N_RANGES {len(ids)}",
     "static const uint16_t can_ids_range_id[CAN_IDS_N_RANGES] = {" + ", ".join(f"0x{i:03X}" for i in ids) + "};",
     "static const uint8_t can_ids_range_lo[CAN_IDS_N_RANGES][5] = {"]
L += ["    {" + ", ".join(map(str, r)) + "}," for r in lo] + ["};",
      "static const uint8_t can_ids_range_hi[CAN_IDS_N_RANGES][5] = {"]
L += ["    {" + ", ".join(map(str, r)) + "}," for r in hi] + ["};", "#endif", ""]
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "mcu", "stm32_ids", "src", "can_ids_ranges.h"), "w").write("\n".join(L))
print(len(ids), "IDs written")
