"""IDS / IPS accuracy of the FPGA classifier on every frame of the dataset.

    py -3.13 python/fpga_accuracy.py                       all frames, all files
    py -3.13 python/fpga_accuracy.py --model models/rf5_d9_p6_time.joblib --rf hdl/generated_p6/rf_ids.v
    add --c-header mcu/stm32_ids/src/can_ids_model_p6.h to check the STM32 C model too
    py -3.13 python/fpga_accuracy.py --limit-per-file 100000   quick try

What runs: every frame of the 4 attack files and the normal file goes through
the real Verilog, hdl/generated/rf_ids.v (the forest) and hdl/known_id.v (the
unknown ID rule), in Icarus, exactly as the FPGA gets them at feat_ready:
ID, DLC (at most 8), and the data bytes the model reads (5, 6 or 8). The classifier is a fixed function of
those values, so each distinct frame is simulated once and its answer is
used for every copy of it (a DoS flood is one frame repeated). The chunks run
in parallel, one per CPU core. Python's forest answers every frame too, and the
script stops if the FPGA disagrees with it on even one.

Not simulated here: the CAN bit level (hdl/can_rx.v). That is checked
separately (run_sim.bat: can_rx_tb.v and dataset frames, all read back
exactly); playing all 17 million frames bit by bit would take ~8 hours.

IPS: on the FPGA the verdict comes 4 clocks (80 ns) after the 5th data byte,
with ~86 us of the frame still to go, so every verdict is in time: a frame
the IDS flags is a frame the error flag destroys. "Attack frames destroyed"
is therefore the detection rate, "normal frames wrongly destroyed" the false
alarm rate. On a real bus a DoS attacker also goes bus off after 32 destroyed
frames and sends nothing more; this counts each frame on its own.

Two scores per file: all frames, and "unseen" frames only (the test part of
the time split the model was trained with, --test-size 0.2 as in train.py:
traffic recorded after everything the model learned from).

Writes build/accuracy/<model name>/report.html, report.csv and confusion.csv.
"""
import argparse
import csv
import os
import subprocess
import sys
import time
from array import array

import numpy as np

from dataset import ATTACK_FILES, CLASS_NAMES, NORMAL_FILE, parse_normal_txt
from forest import hard_vote_predict, known_id_rule

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IVERILOG = r"C:\iverilog\bin\iverilog.exe"
VVP = r"C:\iverilog\bin\vvp.exe"
MAX_CHUNK = 4_000_000          # rf_all_tb.v holds up to 4 M IDs per run


def key_of(cid, dlc, data, nb):
    """The forest's inputs packed in two integers, as the FPGA gets them at
    feat_ready: only the first nb data bytes (the ones the model reads), bytes
    past the DLC 0. hi = ID (11 bits), DLC (4), bytes 0-5; lo = bytes 6-7."""
    d = (data[:min(dlc, nb)] + [0] * 8)[:8]
    hi = (cid & 0x7FF) << 4 | min(dlc, 8)
    for b in d[:6]:
        hi = hi << 8 | b
    return hi, d[6] << 8 | d[7]


def read_attack(path, cls, limit, nb):
    """Keys and labels of one attack CSV, parsed as dataset.parse_attack_csv."""
    keys, lows, labels = array("q"), array("i"), array("b")
    with open(path) as f:
        for line in f:
            p = line.strip().split(",")
            if len(p) < 4:
                continue
            try:
                cid, dlc = int(p[1], 16), int(p[2])
                data = [int(b, 16) for b in p[3:3 + dlc]]
            except ValueError:
                continue
            flag = p[3 + dlc].strip() if len(p) > 3 + dlc else "R"
            hi, lo = key_of(cid, dlc, data, nb)
            keys.append(hi)
            lows.append(lo)
            labels.append(cls if flag == "T" else 0)
            if limit and len(keys) >= limit:
                break
    k = np.stack([np.frombuffer(keys, dtype=np.int64),
                  np.frombuffer(lows, dtype=np.int32).astype(np.int64)], axis=1)
    return k, np.frombuffer(labels, dtype=np.int8)


def read_normal(path, limit, nb):
    X, y = parse_normal_txt(path, limit)
    keys = np.array([key_of(r[0], r[1], list(r[2:2 + r[1]]), nb) for r in X],
                    dtype=np.int64).reshape(-1, 2)
    return keys, np.asarray(y, dtype=np.int8)


def unpack(keys):
    """Keys (n x 2: hi, lo) back to the forest's 10 inputs."""
    hi, lo = keys[:, 0], keys[:, 1]
    X = np.zeros((len(keys), 10), dtype=np.float32)
    X[:, 0] = (hi >> 52) & 0x7FF
    X[:, 1] = (hi >> 48) & 0xF
    for b in range(6):
        X[:, 2 + b] = (hi >> (40 - 8 * b)) & 0xFF
    X[:, 8] = (lo >> 8) & 0xFF
    X[:, 9] = lo & 0xFF
    return X


def c_model(header):
    """Read the STM32's C model (the emlearn header from export_c.py) as C
    sees it: each can_ids_tree_k() as nested `if (features[i] < t) {..} else
    {..}` / `return c;`, the vote in can_ids_predict() (one per tree, the first
    class with the most votes wins) and the 2048-bit can_ids_known[] table.
    Evaluates the text of the header itself, not Python's model, so a wrong
    threshold or leaf in the header shows up as a mismatch."""
    import re
    with open(header) as f:
        src = f.read()
    tok_re = re.compile(r"features\[(\d+)\]\s*<\s*([-0-9.eE+]+)f?|return\s+(\d+)\s*;|\belse\b|[{}]")
    trees = []
    for m in re.finditer(r"can_ids_tree_\d+\s*\([^)]*\)\s*\{", src):
        i, depth = m.end(), 1                 # the function body, braces matched
        while depth:
            depth += {"{": 1, "}": -1}.get(src[i], 0)
            i += 1
        toks = [t for t in tok_re.finditer(src[m.end():i - 1])]
        pos = [0]

        def node():
            t = toks[pos[0]]
            pos[0] += 1
            if t.group(3) is not None:                       # return c;
                return int(t.group(3))
            feat, thr = int(t.group(1)), np.float32(float(t.group(2)))
            pos[0] += 1                                      # {
            left = node()
            pos[0] += 2                                      # } else
            pos[0] += 1                                      # {
            right = node()
            pos[0] += 1                                      # }
            return (feat, thr, left, right)
        trees.append(node())
    table = [int(b, 16) for b in re.findall(r"0x([0-9A-Fa-f]{2})",
             src[src.index("can_ids_known[256]"):].split("};")[0])]
    known = np.array([(table[i >> 3] >> (i & 7)) & 1 for i in range(2048)], dtype=bool)
    return trees, known


def run_c(header, X, expect_forest, expect_final):
    """The STM32's C model on every distinct frame (see c_model). Returns the
    number of frames where its forest class or final class (with the unknown
    ID rule) differs from Python's."""
    trees, known = c_model(header)
    X = X.astype(np.float32)

    def walk(t, rows):
        out = np.empty(len(rows), dtype=np.int8)
        if isinstance(t, int):
            out[:] = t
            return out
        feat, thr, left, right = t
        go_left = X[rows, feat] < thr                       # float compare, as in C
        out[go_left] = walk(left, rows[go_left])
        out[~go_left] = walk(right, rows[~go_left])
        return out

    rows = np.arange(len(X))
    votes = np.zeros((len(X), 5), dtype=np.int32)
    for t in trees:
        votes[rows, walk(t, rows)] += 1
    forest = np.argmax(votes, axis=1).astype(np.int8)        # first maximum, as emlearn's strict >
    final = forest.copy()
    final[(forest == 0) & ~known[X[:, 0].astype(int)]] = 2
    bad = int(((forest != expect_forest) | (final != expect_final)).sum())
    print("STM32 C model (%s, %d trees, %d known IDs) vs Python on %d distinct frames: differs on %d"
          % (os.path.basename(header), len(trees), int(known.sum()), len(X), bad))
    return bad


def known_id_v(path):
    """The IDs in hdl/known_id.v's case statement."""
    import re
    with open(path) as f:
        return sorted(int(h, 16) for h in re.findall(r"11'h([0-9A-Fa-f]{3})", f.read()))


def time_split_test(labels, test_size):
    """train.py --split time for one file: the cut is where test_size of the
    attack frames remain (normal file: at 1 - test_size of its frames)."""
    test = np.zeros(len(labels), dtype=bool)
    attack = np.flatnonzero(labels != 0)
    cut = attack[int(len(attack) * (1 - test_size))] if len(attack) else int(len(labels) * (1 - test_size))
    test[cut:] = True
    return test


def run_fpga(ukeys, work, jobs, rf_path):
    """Every distinct frame through rf_ids.v + known_id.v, chunks in parallel."""
    sim = os.path.join(work, "rf_all_sim")
    cmd = [IVERILOG, "-g2005", "-o", sim, os.path.join(ROOT, "hdl", "rf_all_tb.v"),
           rf_path, os.path.join(ROOT, "hdl", "known_id.v")]
    subprocess.run(cmd, check=True)
    n = len(ukeys)
    n_chunks = max(jobs, -(-n // MAX_CHUNK))
    bounds = np.linspace(0, n, n_chunks + 1).astype(int)
    procs = []
    for c in range(n_chunks):
        part = ukeys[bounds[c]:bounds[c + 1]]
        fin = os.path.join(work, "chunk%02d.hex" % c)
        with open(fin, "w") as f:
            f.write("".join("%016X%04X\n" % (h, l) for h, l in part.tolist()))
        procs.append((c, fin, os.path.join(work, "chunk%02d.out" % c)))
    print("FPGA: %d distinct frames in %d chunks, %d at a time ..." % (n, n_chunks, jobs))
    t0 = time.time()
    running = []
    todo = list(procs)
    while todo or running:
        while todo and len(running) < jobs:
            c, fin, fout = todo.pop(0)
            p = subprocess.Popen([VVP, "-n", sim, "+in=" + fin, "+out=" + fout],
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            running.append((c, p))
        time.sleep(0.5)
        for c, p in list(running):
            if p.poll() is not None:
                out = p.stdout.read()
                if p.returncode != 0 or "DONE" not in out:
                    raise SystemExit("Simulation chunk %d failed:\n%s" % (c, out))
                running.remove((c, p))
                print("  chunk %d done (%.0f s)" % (c, time.time() - t0))
    forest, final = [], []
    for c, fin, fout in procs:
        with open(fout) as f:
            lines = f.read().split()
        if len(lines) != bounds[c + 1] - bounds[c]:
            raise SystemExit("Chunk %d: %d answers for %d frames" % (c, len(lines), bounds[c + 1] - bounds[c]))
        forest += [int(s[0]) for s in lines]
        final += [int(s[1]) for s in lines]
        os.remove(fin)
        os.remove(fout)
    return np.array(forest, dtype=np.int8), np.array(final, dtype=np.int8)


def scores(label, pred):
    """IDS and IPS numbers for one set of frames."""
    att, nor = label != 0, label == 0
    r = {"frames": len(label), "normal": int(nor.sum()), "attack": int(att.sum()),
         "accuracy": float((pred == label).mean()) if len(label) else float("nan"),
         "attack_flagged": int((pred[att] != 0).sum()), "normal_flagged": int((pred[nor] != 0).sum())}
    r["detection"] = r["attack_flagged"] / r["attack"] if r["attack"] else float("nan")
    r["false_alarm"] = r["normal_flagged"] / r["normal"] if r["normal"] else float("nan")
    for c, name in enumerate(CLASS_NAMES):
        m = label == c
        r["recall_" + name] = float((pred[m] == c).mean()) if m.any() else float("nan")
    return r


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--model", default=os.path.join(ROOT, "models", "rf5_d9_p5_time.joblib"))
    ap.add_argument("--limit-per-file", type=int, default=0, help="0 = every frame")
    ap.add_argument("--test-size", type=float, default=0.2, help="as the model was trained")
    ap.add_argument("--rf", default=os.path.join(ROOT, "hdl", "generated", "rf_ids.v"),
                    help="the rf_ids.v exported from --model")
    ap.add_argument("--c-header",
                    help="also check the STM32's C model (mcu/stm32_ids/src/can_ids_model*.h) on every distinct frame")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument("--out", help="default build/accuracy/<model name>")
    args = ap.parse_args()
    model_name = os.path.splitext(os.path.basename(args.model))[0]
    args.out = args.out or os.path.join(ROOT, "build", "accuracy", model_name)
    os.makedirs(args.out, exist_ok=True)
    t_start = time.time()

    import joblib
    bundle = joblib.load(args.model)
    nb = int(bundle.get("payload_bytes", 8))
    model_known = sorted(int(i) for i in bundle.get("known_ids", []))
    if known_id_v(os.path.join(ROOT, "hdl", "known_id.v")) != model_known:
        raise SystemExit("hdl/known_id.v does not list this model's %d known IDs: update it first" % len(model_known))
    print("model %s: reads ID, DLC and %d data bytes; %d known IDs (known_id.v matches)"
          % (model_name, nb, len(model_known)))

    # 1. Every frame of every file.
    names, keys, labels, tests = [], [], [], []
    files = [(n, c) for n, c in ATTACK_FILES.items()] + [(NORMAL_FILE, None)]
    for name, cls in files:
        path = os.path.join(args.data, name)
        if not os.path.exists(path):
            print("skipped (missing): %s" % path)
            continue
        t0 = time.time()
        k, y = read_attack(path, cls, args.limit_per_file, nb) if cls else \
            read_normal(path, args.limit_per_file or None, nb)
        names.append(name)
        keys.append(k)
        labels.append(y)
        tests.append(time_split_test(y, args.test_size))
        print("read %-22s %9d frames (%d attack) in %.0f s" % (name, len(k), int((y != 0).sum()), time.time() - t0))
    n_total = sum(len(k) for k in keys)
    n_test = sum(int(t.sum()) for t in tests)

    if not args.limit_per_file:
        print("unseen (test) frames: %d; the model's own test set: %d %s"
              % (n_test, len(bundle["y_test"]), "(same split)" if n_test == len(bundle["y_test"]) else "(DIFFERENT)"))

    # 2. Distinct frames, through Python's forest and through the Verilog.
    all_keys = np.concatenate(keys)
    ukeys, inv = np.unique(all_keys, axis=0, return_inverse=True)
    inv = inv.ravel()
    Xu = unpack(ukeys)
    py_forest = hard_vote_predict(bundle["model"], Xu).astype(np.int8)
    py_final = known_id_rule(py_forest, Xu[:, 0].astype(int), bundle.get("known_ids", [])).astype(np.int8)
    fpga_forest, fpga_final = run_fpga(ukeys, args.out, args.jobs, args.rf)
    bad_forest = int((fpga_forest != py_forest).sum())
    bad_final = int((fpga_final != py_final).sum())
    print("FPGA vs Python on %d distinct frames: forest differs on %d, final on %d"
          % (len(ukeys), bad_forest, bad_final))
    if bad_forest or bad_final:
        raise SystemExit("The Verilog does not match Python: the accuracy below would not be the FPGA's.")
    if args.c_header:
        bad_c = run_c(args.c_header, Xu, py_forest, py_final)
        if bad_c:
            raise SystemExit("The STM32's C model does not match Python on %d frames." % bad_c)

    # 3. Scores per file, from the FPGA's final answer for every frame.
    pred_all = fpga_final[inv]
    rows, pos = [], 0
    for name, y, t in zip(names, labels, tests):
        p = pred_all[pos:pos + len(y)]
        pos += len(y)
        for part, m in (("all frames", np.ones(len(y), bool)), ("unseen only", t)):
            rows.append(dict(file=name, set=part, **scores(y[m], p[m])))
    y_all, t_all = np.concatenate(labels), np.concatenate(tests)
    for part, m in (("all frames", np.ones(len(y_all), bool)), ("unseen only", t_all)):
        rows.append(dict(file="ALL FILES", set=part, **scores(y_all[m], pred_all[m])))

    conf = {}
    for part, m in (("all frames", np.ones(len(y_all), bool)), ("unseen only", t_all)):
        cm = np.zeros((5, 5), dtype=np.int64)
        np.add.at(cm, (y_all[m], pred_all[m]), 1)
        conf[part] = cm

    with open(os.path.join(args.out, "report.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    with open(os.path.join(args.out, "confusion.csv"), "w", newline="") as f:
        w = csv.writer(f)
        for part, cm in conf.items():
            w.writerow([part, "true \\ FPGA says"] + CLASS_NAMES)
            for c, name in enumerate(CLASS_NAMES):
                w.writerow(["", name] + cm[c].tolist())
    write_html(os.path.join(args.out, "report.html"), rows, conf, n_total, len(ukeys),
               time.time() - t_start, model_name, nb)

    print("\n%-20s %-12s %9s %10s %16s %16s" % ("file", "set", "frames", "accuracy",
          "attacks stopped", "normal blocked"))
    for r in rows:
        print("%-20s %-12s %9d %10s %16s %16s"
              % (r["file"], r["set"], r["frames"], pct(r["accuracy"]),
                 pct(r["detection"]), "%s (%d)" % (pct(r["false_alarm"], 4), r["normal_flagged"])
                 if r["normal"] else "-"))
    print("\nReport: %s (%.0f s)" % (os.path.join(args.out, "report.html"), time.time() - t_start))


def pct(x, digits=3):
    return "-" if x != x else ("%." + str(digits) + "f %%") % (100 * x)


def write_html(path, rows, conf, n_total, n_unique, secs, model_name, nb):
    left = 19 + 8 * (8 - nb)                     # bit times after the last byte read
    h = ["<!doctype html><meta charset='utf-8'><title>FPGA IDS/IPS accuracy</title>",
         "<style>body{font-family:Segoe UI,Arial,sans-serif;font-size:14px;margin:20px;color:#1b1f24;background:#fff;max-width:1200px}"
         "table{border-collapse:collapse;margin:8px 0 18px}th,td{border:1px solid #ccd;padding:4px 8px;text-align:right}"
         "th{background:#e9eef5}td.l,th.l{text-align:left}tr.tot td{font-weight:600;background:#f4f6f9}"
         "td.diag{background:#e6f4e6}p.note{color:#555}</style>",
         "<h1>FPGA IDS/IPS accuracy on the whole dataset</h1>",
         "<p>Model <b>%s</b>: reads the ID, DLC and the first <b>%d</b> data bytes.</p>" % (model_name, nb),
         "<p>%d frames from %d files. Every distinct frame (%d) ran through the Verilog classifier "
         "(rf_ids.v + known_id.v) in Icarus, and the FPGA agreed with Python's forest on every one. "
         "Run time %.0f s.</p>" % (n_total, len({r['file'] for r in rows}) - 1, n_unique, secs),
         "<p class='note'>IPS: the FPGA's verdict is always in time (80 ns after data byte %d, with at "
         "least %d bit times = %d &micro;s of the frame left)" % (nb, left, 2 * left) + ", so a flagged attack frame is a destroyed frame, and a "
         "flagged normal frame is a normal frame wrongly destroyed. Each frame is counted on its own "
         "(on a real bus a DoS attacker also goes bus off). <b>Unseen only</b> = the frames recorded after "
         "the model's training part (time split, 20 %): the fair score.</p>"]
    for part in ("unseen only", "all frames"):
        h.append("<h2>%s</h2><table><tr><th class='l'>file</th><th>frames</th><th>normal</th><th>attack</th>"
                 "<th>accuracy</th><th>attack frames destroyed</th><th>normal frames wrongly destroyed</th>"
                 "%s</tr>" % (part.capitalize(), "".join("<th>%s recall</th>" % n for n in CLASS_NAMES)))
        for r in rows:
            if r["set"] != part:
                continue
            h.append("<tr class='%s'><td class='l'>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%s</td>"
                     "<td>%s (%d)</td><td>%s (%d)</td>%s</tr>"
                     % ("tot" if r["file"] == "ALL FILES" else "", r["file"], r["frames"], r["normal"],
                        r["attack"], pct(r["accuracy"]), pct(r["detection"]), r["attack_flagged"],
                        pct(r["false_alarm"], 4), r["normal_flagged"],
                        "".join("<td>%s</td>" % pct(r["recall_" + n]) for n in CLASS_NAMES)))
        h.append("</table>")
        cm = conf[part]
        h.append("<h3>Confusion matrix, %s (rows: true class, columns: what the FPGA says)</h3><table>"
                 "<tr><th class='l'>true \\ FPGA</th>%s</tr>" % (part, "".join("<th>%s</th>" % n for n in CLASS_NAMES)))
        for c, name in enumerate(CLASS_NAMES):
            h.append("<tr><td class='l'>%s</td>%s</tr>" % (name, "".join(
                "<td class='%s'>%d</td>" % ("diag" if c == k else "", cm[c, k]) for k in range(5))))
        h.append("</table>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(h))


if __name__ == "__main__":
    main()
