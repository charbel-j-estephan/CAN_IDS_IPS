"""Turn bench run folders into the numbers for the paper's Table VIII.

Usage (from the project folder):
    py -3.13 docs\\analysis_scripts\\bench_rows.py runs\\20261124_*
    py -3.13 docs\\analysis_scripts\\bench_rows.py runs\\20261124_093000_mixed runs\\...

For each run it prints:
  attack frames blocked   = attack frames sent that never reached the receiver
  genuine delivered       = genuine frames the receiver heard (receiver count,
                            minus the start marker, capped at what was sent)
  sender state and tec    = from the sender's own report
  attacker busoffs and recoveries, when the attacker logged them
  flags                   = the STM32's error flag count, when the STM32 was the IPS
                            (for the FPGA, read the counters step instead)
"""
import glob
import json
import os
import sys


def row(folder):
    p = os.path.join(folder, "summary.json")
    if not os.path.exists(p):
        return None
    d = json.load(open(p))
    bc = d.get("by_class", {})
    att_sent = sum(v["sent"] for k, v in bc.items() if k != "Normal")
    att_arr = sum(v["arrived"] for k, v in bc.items() if k != "Normal")
    norm = bc.get("Normal", {"sent": 0, "arrived": 0})
    nodes = d.get("nodes", {})
    snd = nodes.get("sender", {})
    rcv = nodes.get("receiver", {})
    att = nodes.get("attacker", {})
    ips = nodes.get("STM32 IPS", {})
    # The receiver count is the record. The PC's frame matching can miss a few
    # frames after a sender stall, so prefer the receiver count when it is higher.
    marker = 1 if snd.get("sent", 0) == norm["sent"] + 1 else 0
    heard = max(0, rcv.get("rx", 0) - marker - att_arr)
    delivered = min(norm["sent"], max(norm["arrived"], heard))
    return {
        "run": os.path.basename(folder.rstrip("\\/")),
        "file": d.get("file"),
        "seed": d.get("mix_seed", ""),
        "frames": d.get("frames_sent"),
        "per_class": {k: "%d/%d" % (v["sent"] - v["arrived"], v["sent"])
                      for k, v in bc.items() if k != "Normal"},
        "blocked": "%d / %d" % (att_sent - att_arr, att_sent),
        "genuine": "%d / %d" % (delivered, norm["sent"]),
        "sender": "%s, TEC %s" % (snd.get("state", "?"), snd.get("tec", "?")),
        "attacker": ("busoffs %s, recovered %s" % (att["busoffs"], att["recovered"])
                     if "busoffs" in att else "not logged"),
        "flags": ips.get("rej", "see FPGA counters"),
    }


def main():
    folders = []
    for a in sys.argv[1:] or ["runs/*"]:
        folders += sorted(glob.glob(a))
    for f in folders:
        r = row(f)
        if not r:
            continue
        print("%s  (%s, seed %s, %s frames)" % (r["run"], r["file"], r["seed"], r["frames"]))
        print("   attack blocked  %s   %s" % (r["blocked"], r["per_class"]))
        print("   genuine         %s" % r["genuine"])
        print("   flags           %s" % r["flags"])
        print("   sender          %s" % r["sender"])
        print("   attacker        %s" % r["attacker"])
        print()


if __name__ == "__main__":
    main()
