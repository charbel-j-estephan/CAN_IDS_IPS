"""Put what was sent next to what the FPGA read, frame by frame.

    py -3.13 python/compare_sim.py

Reads build/sim/sent.csv (python/make_sim_frames.py) and build/sim/got.csv
(hdl/frames_tb.v), writes build/sim/compare.csv and build/sim/compare.html,
and prints a summary with the first mismatches.

Every event in got.csv carries the bus bit that was being played when it
happened, so it belongs to the frame whose start_bit comes last before it.
Each frame must give exactly one FRAME with its ID, DLC and data, and when the
log has FEAT and CLASS events, exactly one of each: the forest values must be
what the forest should see, and the class must be Python's forest class.
The dataset label is shown for information only: a forest answer that differs
from the label is the model's miss, not a Verilog bug.
"""
import argparse
import bisect
import csv
import html
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLASS_NAMES = ["Normal", "DoS", "Fuzzy", "Gear_spoof", "RPM_spoof"]


def ascii_of(data):
    return "".join(chr(b) if 32 <= b < 127 else "." for b in data)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dir", default=os.path.join(ROOT, "build", "sim"))
    ap.add_argument("--show", type=int, default=10, help="mismatches printed in full")
    args = ap.parse_args()

    with open(os.path.join(args.dir, "sent.csv")) as f:
        sent = list(csv.DictReader(f))
    with open(os.path.join(args.dir, "got.csv")) as f:
        got = list(csv.DictReader(f))

    starts = [int(s["start_bit"]) for s in sent]
    feat_cols = [c for c in sent[0] if c.startswith("f_")]   # f_id, f_dlc, f_d0 .. f_d<n-1>
    nb = len(feat_cols) - 2                                  # data bytes the model reads
    has_feat = any(e["event"] == "FEAT" for e in got)
    has_class = any(e["event"] == "CLASS" for e in got)
    has_final = any(e["event"] == "FINAL" for e in got)

    # Events per frame.
    per = [[] for _ in sent]
    before = []                                     # events before the first frame
    for e in got:
        k = bisect.bisect_right(starts, int(e["bit"])) - 1
        (per[k] if k >= 0 else before).append(e)

    rows, n_bad = [], 0
    for s, events in zip(sent, per):
        dlc = int(s["dlc"])
        sent_data = [int(b, 16) for b in s["data"].split()] if s["data"] else []
        sent_feat = [int(s[c]) for c in feat_cols]
        kinds = [e["event"] for e in events]
        problems = []

        frames = [e for e in events if e["event"] == "FRAME"]
        g_id = g_dlc = g_data = ""
        if len(frames) != 1:
            problems.append("%d FRAME events (expected 1)" % len(frames))
        if frames:
            e = frames[0]
            gd = bytes.fromhex(e["data"].rjust(16, "0"))
            g_id, g_dlc = e["id"].upper().rjust(3, "0"), int(e["dlc"])
            g_data = list(gd[:min(g_dlc, 8)])
            if g_id != s["id"]:
                problems.append("ID %s, sent %s" % (g_id, s["id"]))
            if g_dlc != dlc:
                problems.append("DLC %d, sent %d" % (g_dlc, dlc))
            if g_data != sent_data:
                problems.append("data differs")
            if any(gd[min(g_dlc, 8):]):
                problems.append("bytes past the DLC are not 0")
        for err in ("CRC_ERR", "STUFF_ERR"):
            if err in kinds:
                problems.append("%d %s" % (kinds.count(err), err))

        g_feat = ""
        if has_feat:
            feats = [e for e in events if e["event"] == "FEAT"]
            if len(feats) != 1:
                problems.append("%d FEAT events (expected 1)" % len(feats))
            if feats:
                e = feats[0]
                gd = bytes.fromhex(e["data"].rjust(16, "0"))
                g_feat = [int(e["id"], 16), min(int(e["dlc"]), 8)] + list(gd[:nb])
                if g_feat != sent_feat:
                    problems.append("forest values differ")
                if any(gd[nb:]):
                    problems.append("bytes past the first %d not 0 at FEAT" % nb)

        g_cls = ""
        if has_class:
            cls = [e for e in events if e["event"] == "CLASS"]
            if len(cls) != 1:
                problems.append("%d CLASS events (expected 1)" % len(cls))
            if cls:
                c = int(cls[0]["class"])
                g_cls = CLASS_NAMES[c] if c < len(CLASS_NAMES) else str(c)
                if g_cls != s["forest"]:
                    problems.append("class %s, Python's forest says %s" % (g_cls, s["forest"]))

        g_final = ""
        if has_final:
            fin = [e for e in events if e["event"] == "FINAL"]
            if len(fin) != 1:
                problems.append("%d FINAL events (expected 1)" % len(fin))
            if fin:
                c = int(fin[0]["class"])
                g_final = CLASS_NAMES[c] if c < len(CLASS_NAMES) else str(c)
                if g_final != s["final"]:
                    problems.append("final %s, Python says %s (unknown ID rule)" % (g_final, s["final"]))

        n_bad += bool(problems)
        rows.append({
            "n": s["n"], "start_bit": s["start_bit"], "label": s["label"],
            "sent_id": s["id"], "sent_dlc": dlc, "sent_data": s["data"], "sent_chars": s["chars"],
            "got_id": g_id, "got_dlc": g_dlc,
            "got_data": " ".join("%02X" % b for b in g_data) if g_data != "" else "",
            "got_chars": ascii_of(g_data) if g_data != "" else "",
            "forest_should_see": " ".join(map(str, sent_feat)),
            "forest_saw": " ".join(map(str, g_feat)) if g_feat != "" else "",
            "python_forest": s["forest"], "fpga_forest": g_cls,
            "python_final": s["final"], "fpga_final": g_final,
            "events": " ".join(kinds), "result": "OK" if not problems else "; ".join(problems),
        })

    with open(os.path.join(args.dir, "compare.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    write_html(os.path.join(args.dir, "compare.html"), rows, has_feat, has_class, has_final,
               n_bad, before, nb)

    checked = ["ID, DLC, data"] + (["forest values"] if has_feat else []) + \
              (["forest class"] if has_class else []) + \
              (["final class (unknown ID rule)"] if has_final else [])
    print("%d frames sent, %d events logged. Checked: %s."
          % (len(sent), len(got), ", ".join(checked)))
    if not has_feat:
        print("(No FEAT events: build the testbench with -DHAS_FEAT once can_rx has feat_ready.)")
    if before:
        print("%d events before the first frame started (should be 0)." % len(before))
    if n_bad == 0 and not before:
        print("PASS: every frame read back exactly as sent.")
    else:
        print("FAIL: %d of %d frames differ. First ones:" % (n_bad, len(sent)))
        for r in [r for r in rows if r["result"] != "OK"][:args.show]:
            print("  frame %s (bus bit %s, %s): %s" % (r["n"], r["start_bit"], r["label"], r["result"]))
            print("     sent   %s [%s] %-23s %s" % (r["sent_id"], r["sent_dlc"], r["sent_data"], r["sent_chars"]))
            print("     read   %s [%s] %-23s %s" % (r["got_id"] or "---", r["got_dlc"], r["got_data"], r["got_chars"]))
            print("     events %s" % (r["events"] or "none"))
    print("Full table: %s" % os.path.join(args.dir, "compare.html"))


def write_html(path, rows, has_feat, has_class, has_final, n_bad, before, nb):
    cols = [("n", "#"), ("start_bit", "bus bit"), ("label", "dataset label"),
            ("sent_id", "sent ID"), ("got_id", "read ID"),
            ("sent_dlc", "sent DLC"), ("got_dlc", "read DLC"),
            ("sent_data", "sent data"), ("got_data", "read data"),
            ("sent_chars", "sent chars"), ("got_chars", "read chars"),
            ("forest_should_see", "forest should see (id dlc d0-d%d)" % (nb - 1))]
    if has_feat:
        cols.append(("forest_saw", "forest saw"))
    cols.append(("python_forest", "Python forest"))
    if has_class:
        cols.append(("fpga_forest", "FPGA forest"))
    cols.append(("python_final", "Python final (ID rule)"))
    if has_final:
        cols.append(("fpga_final", "FPGA final"))
    cols += [("events", "events"), ("result", "result")]
    out = ["<!doctype html><meta charset='utf-8'><title>FPGA sim compare</title>",
           "<style>body{font-family:Segoe UI,Arial,sans-serif;font-size:13px;margin:16px;background:#fff;color:#1b1f24}"
           "table{border-collapse:collapse}th,td{border:1px solid #ccd;padding:3px 6px;white-space:nowrap}"
           "th{background:#e9eef5;position:sticky;top:0}td.m{font-family:Consolas,monospace}"
           "tr.bad td{background:#fde8e8}tr.bad td.r{color:#a00;font-weight:600}</style>",
           "<h2>Sent vs read: %d frames, %s</h2>" % (len(rows), "all match" if not n_bad else "%d differ" % n_bad)]
    if before:
        out.append("<p>%d events came before the first frame started.</p>" % len(before))
    out.append("<table><tr>%s</tr>" % "".join("<th>%s</th>" % html.escape(h) for _, h in cols))
    mono = {"sent_id", "got_id", "sent_data", "got_data", "sent_chars", "got_chars",
            "forest_should_see", "forest_saw"}
    for r in rows:
        cells = []
        for k, _ in cols:
            cls = "m" if k in mono else ("r" if k == "result" else "")
            cells.append("<td class='%s'>%s</td>" % (cls, html.escape(str(r[k]))))
        out.append("<tr class='%s'>%s</tr>" % ("" if r["result"] == "OK" else "bad", "".join(cells)))
    out.append("</table>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))


if __name__ == "__main__":
    main()
