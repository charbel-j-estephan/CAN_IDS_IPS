"""Write a small FAKE dataset in the Car Hacking file format.

Use it only to check that the toolchain runs before you download the real
dataset. Accuracy numbers from this data mean nothing.
"""
import argparse
import os
import random


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data/demo")
    ap.add_argument("--frames", type=int, default=20000)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    rng = random.Random(args.seed)
    os.makedirs(args.out, exist_ok=True)

    ids = [0x316, 0x18F, 0x260, 0x2A0, 0x329, 0x545, 0x43F, 0x4F0, 0x5F0, 0x153]

    def normal():
        cid = rng.choice(ids)
        dlc = 2 if cid == 0x5F0 else 8
        return cid, [rng.randrange(0, 0x40) for _ in range(dlc)]

    def write_attack(name, attack):
        ts = 1478198376.0
        with open(os.path.join(args.out, name), "w") as f:
            for _ in range(args.frames):
                ts += 0.0005
                if rng.random() < 0.3:
                    cid, data = attack()
                    flag = "T"
                else:
                    cid, data = normal()
                    flag = "R"
                fields = ["%.6f" % ts, "%04x" % cid, str(len(data))]
                fields += ["%02x" % b for b in data] + [flag]
                f.write(",".join(fields) + "\n")

    write_attack("DoS_dataset.csv", lambda: (0x000, [0] * 8))
    write_attack("Fuzzy_dataset.csv", lambda: (
        rng.randrange(0x800), [rng.randrange(256) for _ in range(8)]))
    write_attack("gear_dataset.csv", lambda: (
        0x43F, [0x01, 0x45, 0x60, 0xFF, 0x6B, 0x00, 0x00, 0x00]))
    write_attack("RPM_dataset.csv", lambda: (
        0x316, [0x45, 0x29, 0x24, 0xFF, 0x29, 0x24, 0x00, 0xFF]))

    ts = 1479121434.0
    with open(os.path.join(args.out, "normal_run_data.txt"), "w") as f:
        for _ in range(args.frames):
            ts += 0.0005
            cid, data = normal()
            f.write("Timestamp: %.6f        ID: %04x    000    DLC: %d    %s\n"
                    % (ts, cid, len(data), " ".join("%02x" % b for b in data)))
    print("Wrote fake demo data to", args.out)


if __name__ == "__main__":
    main()
