"""Test vectors for the FPGA CAN receiver (can_rx): real dataset frames turned
into the exact bit stream on the bus, plus the fields can_rx must output.

    python python/make_can_vectors.py                  1000 frames from the attack files
    python python/make_can_vectors.py --frames 5000

Writes hdl/vectors/:
  bus_bits.mem   one bus bit per line, 0 = dominant, 1 = recessive, as RXD shows it
  expected.mem   one line per frame, 22 hex digits:
                   flags(2) can_id(3) dlc(1) data(16)
                   flags: 00 good frame, 01 CRC error, 02 stuff error
  can_vectors.vh N_BITS and N_FRAMES for the testbench

Every frame is built the way a CAN controller sends it: SOF, 11 bit ID, RTR,
IDE, r0, DLC, data, CRC-15, with a stuff bit after every 5 equal bits from SOF
to the end of the CRC, then CRC delimiter, ACK slot (dominant, as if another
node acknowledged), ACK delimiter, 7 bit EOF and 3 bit interframe space,
followed by a random idle gap. A few frames are broken on purpose, one with a
wrong CRC and one with 6 equal bits, to check the error outputs.

The stream is checked by an independent decoder in this script before it is
written, so a testbench failure means the receiver, not the vectors.
"""
import argparse
import os
import random

from can_bits import _crc15

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILES = ["DoS_dataset.csv", "Fuzzy_dataset.csv", "gear_dataset.csv", "RPM_dataset.csv"]
GOOD, CRC_ERR, STUFF_ERR = 0, 1, 2


def unstuffed_bits(can_id, dlc, data):
    """SOF to the end of the CRC, before stuffing."""
    bits = [0]
    bits += [(can_id >> (10 - i)) & 1 for i in range(11)]
    bits += [0, 0, 0]                                         # RTR, IDE, r0
    bits += [(dlc >> (3 - i)) & 1 for i in range(4)]
    for b in data[:min(dlc, 8)]:
        bits += [(b >> (7 - i)) & 1 for i in range(8)]
    crc = _crc15(bits)
    return bits + [(crc >> (14 - i)) & 1 for i in range(15)]


def stuff(bits):
    out, run, last = [], 0, None
    for b in bits:
        out.append(b)
        run = run + 1 if b == last else 1
        last = b
        if run == 5:
            out.append(1 - b)
            last, run = 1 - b, 1
    return out


def frame_on_bus(can_id, dlc, data):
    """Whole frame as RXD sees it, from SOF to the end of the interframe space."""
    return stuff(unstuffed_bits(can_id, dlc, data)) + [1, 0, 1] + [1] * 7 + [1] * 3


def decode(stream):
    """Independent reference decoder, used only to check the vectors.

    Waits for 11 recessive bits (bus idle), then a dominant SOF, destuffs,
    parses and checks the CRC. Returns (flag, can_id, dlc, data) per frame."""
    out, i, n, idle = [], 0, len(stream), 0
    while i < n:
        if stream[i] == 1:
            idle += 1
            i += 1
            continue
        if idle < 11:                      # not a SOF: still inside something
            idle = 0
            i += 1
            continue
        idle = 0
        bits, run, last, j, err = [], 0, None, i, None
        need = 19                          # SOF..DLC, extended once DLC is known
        while j < n:
            b = stream[j]
            j += 1
            if run == 5:                   # this one is a stuff bit
                if b == last:
                    err = STUFF_ERR
                    break
                last, run = b, 1
                continue
            bits.append(b)
            run = run + 1 if b == last else 1
            last = b
            if len(bits) == 19:
                dlc_v = int("".join(map(str, bits[15:19])), 2)
                need = 19 + 8 * min(dlc_v, 8) + 15
            if len(bits) == need and len(bits) > 19:
                break
        if err == STUFF_ERR:
            out.append((STUFF_ERR, 0, 0, []))
            i = j
            continue
        cid = int("".join(map(str, bits[1:12])), 2)
        dlc_v = int("".join(map(str, bits[15:19])), 2)
        nb = min(dlc_v, 8)
        data = [int("".join(map(str, bits[19 + 8 * k:27 + 8 * k])), 2) for k in range(nb)]
        crc = int("".join(map(str, bits[19 + 8 * nb:34 + 8 * nb])), 2)
        ok = crc == _crc15(bits[:19 + 8 * nb])
        out.append((GOOD if ok else CRC_ERR, cid, dlc_v, data))
        i = j
    return out


def read_frames(data_dir, per_file, rng):
    frames = []
    for name in FILES:
        path = os.path.join(data_dir, name)
        if not os.path.exists(path):
            continue
        rows = []
        with open(path) as f:
            for line in f:
                p = line.strip().split(",")
                try:
                    cid, dlc = int(p[1], 16), int(p[2])
                    data = [int(b, 16) for b in p[3:3 + dlc]]
                except (ValueError, IndexError):
                    continue
                rows.append((cid & 0x7FF, min(dlc, 8), data[:8]))
                if len(rows) >= per_file * 20:
                    break
        frames += rng.sample(rows, min(per_file, len(rows)))
    if not frames:
        raise SystemExit("No dataset files found in %s" % data_dir)
    return frames


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default=os.path.join(ROOT, "data", "car_hacking"))
    ap.add_argument("--frames", type=int, default=1000)
    ap.add_argument("--out", default=os.path.join(ROOT, "hdl", "vectors"))
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    rng = random.Random(args.seed)

    frames = read_frames(args.data, max(1, args.frames // len(FILES)), rng)
    rng.shuffle(frames)
    # Corner cases first: every DLC, extreme stuffing, extreme IDs.
    corner = [(0x000, 8, [0] * 8), (0x7FF, 8, [0xFF] * 8), (0x555, 8, [0x55, 0xAA] * 4),
              (0x123, 0, [])] + [(0x100 + d, d, list(range(1, d + 1))) for d in range(1, 9)]
    frames = corner + frames

    stream = [1] * 20                      # bus idle at start
    expected = []
    for k, (cid, dlc, data) in enumerate(frames):
        if k == len(frames) // 3:          # a frame whose CRC does not match its data
            bits = unstuffed_bits(cid, dlc, data)
            bits[19 if dlc else 5] ^= 1    # flip a data bit (or an ID bit) after the CRC was made
            stream += stuff(bits) + [1, 0, 1] + [1] * 10
            expected.append((CRC_ERR, cid, dlc, data))
        elif k == 2 * len(frames) // 3:    # six equal bits: a stuff error in the identifier
            stream += [0] + [0] * 6 + [1] * 25
            expected.append((STUFF_ERR, 0, 0, []))
        else:
            stream += frame_on_bus(cid, dlc, data)
            expected.append((GOOD, cid, dlc, data))
        stream += [1] * rng.randint(0, 40)  # idle gap, sometimes none (back to back)

    # Check the stream with the independent decoder before writing anything.
    got = decode(stream)
    want = [(f, c, d, list(x[:min(d, 8)])) if f == GOOD else (f,) for f, c, d, x in expected]
    have = [g if g[0] == GOOD else (g[0],) for g in got]
    if have != want:
        bad = next(i for i, (a, b) in enumerate(zip(have, want)) if a != b) if len(have) == len(want) else -1
        raise SystemExit("Internal check failed (frame %d): the vectors are wrong, not written" % bad)

    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "bus_bits.mem"), "w") as f:
        f.write("\n".join(str(b) for b in stream) + "\n")
    with open(os.path.join(args.out, "expected.mem"), "w") as f:
        for flag, cid, dlc, data in expected:
            d = (list(data[:min(dlc, 8)]) + [0] * 8)[:8] if flag == GOOD else [0] * 8
            c, l = (cid, dlc) if flag == GOOD else (0, 0)
            f.write("%02X%03X%01X%s\n" % (flag, c, l, "".join("%02X" % b for b in d)))
    with open(os.path.join(args.out, "can_vectors.vh"), "w") as f:
        f.write("// Generated by python/make_can_vectors.py\n")
        f.write("localparam N_BITS = %d;\nlocalparam N_FRAMES = %d;\n" % (len(stream), len(expected)))
    n_good = sum(1 for e in expected if e[0] == GOOD)
    print("Wrote %d frames (%d good, 1 CRC error, 1 stuff error), %d bus bits = %.1f ms of bus "
          "time, to %s" % (len(expected), n_good, len(stream), len(stream) * 0.002, args.out))
    print("Checked against the reference decoder: all frames decode as expected.")


if __name__ == "__main__":
    main()
