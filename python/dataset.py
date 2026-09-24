"""Load the HCRL Car Hacking Dataset into per frame integer features.

Download the dataset from https://ocslab.hksecurity.net/Datasets/car-hacking-dataset
and unzip it into one folder. The loader expects these files:

    DoS_dataset.csv, Fuzzy_dataset.csv, gear_dataset.csv, RPM_dataset.csv
    normal_run_data.txt

Attack files mark each frame with a flag, R for a normal frame and T for an
injected frame. Every T frame gets the attack class of its file, every R frame
is labelled Normal.

Each frame becomes 10 integer features: the 11 bit CAN ID, the 4 bit DLC and
the 8 data bytes (missing bytes are 0). Integers keep the FPGA comparators
small and let the FPGA, the C model and Python agree bit for bit.
"""
import os
import re

import numpy as np

CLASS_NAMES = ["Normal", "DoS", "Fuzzy", "Gear_spoof", "RPM_spoof"]

ATTACK_FILES = {
    "DoS_dataset.csv": 1,
    "Fuzzy_dataset.csv": 2,
    "gear_dataset.csv": 3,
    "RPM_dataset.csv": 4,
}
NORMAL_FILE = "normal_run_data.txt"

# (name, bit width). Order here is the feature order everywhere downstream.
FEATURES = [("can_id", 11), ("dlc", 4)] + [("d%d" % i, 8) for i in range(8)]

_NORMAL_RE = re.compile(
    r"ID:\s*([0-9a-fA-F]+)\s+\S+\s+DLC:\s*(\d+)\s*((?:[0-9a-fA-F]{2}\s*)*)$"
)


def _frame(can_id, dlc, data):
    row = [can_id & 0x7FF, min(dlc, 8)]
    data = data[:8]
    return row + data + [0] * (8 - len(data))


def parse_attack_csv(path, attack_class, limit=None):
    """Parse one attack CSV. Rows are ts,id,dlc,byte0..byteN,flag."""
    X, y = [], []
    with open(path) as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) < 4:
                continue
            try:
                can_id = int(parts[1], 16)
                dlc = int(parts[2])
                data = [int(b, 16) for b in parts[3:3 + dlc]]
            except ValueError:
                continue
            flag = parts[3 + dlc].strip() if len(parts) > 3 + dlc else "R"
            X.append(_frame(can_id, dlc, data))
            y.append(attack_class if flag == "T" else 0)
            if limit and len(X) >= limit:
                break
    return X, y


def parse_normal_txt(path, limit=None):
    X, y = [], []
    with open(path) as f:
        for line in f:
            m = _NORMAL_RE.search(line.strip())
            if not m:
                continue
            dlc = int(m.group(2))
            data = [int(b, 16) for b in m.group(3).split()][:dlc]
            X.append(_frame(int(m.group(1), 16), dlc, data))
            y.append(0)
            if limit and len(X) >= limit:
                break
    return X, y


def load(data_dir, limit_per_file=None):
    """Return X (int32, n x 10) and y (int32) for every file found in data_dir."""
    X, y = [], []
    found = []
    for name, cls in ATTACK_FILES.items():
        path = os.path.join(data_dir, name)
        if os.path.exists(path):
            fx, fy = parse_attack_csv(path, cls, limit_per_file)
            X += fx
            y += fy
            found.append(name)
    path = os.path.join(data_dir, NORMAL_FILE)
    if os.path.exists(path):
        fx, fy = parse_normal_txt(path, limit_per_file)
        X += fx
        y += fy
        found.append(NORMAL_FILE)
    if not X:
        raise FileNotFoundError(
            "No Car Hacking files found in %s. Expected %s"
            % (data_dir, ", ".join(list(ATTACK_FILES) + [NORMAL_FILE]))
        )
    print("Loaded %d frames from %s" % (len(X), ", ".join(found)))
    return np.asarray(X, dtype=np.int32), np.asarray(y, dtype=np.int32)
