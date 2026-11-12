"""The generic engine against Python on every distinct frame of the dataset.

    py -3.13 accuracy_engine.py --bytes 5 6 8

Each distinct frame runs through the real Verilog (rf_classifier.v in Icarus,
the model loaded from its blob), in chunks, one per CPU core. Python's forest
(sklearn trees, one vote each) and the unknown ID rule answer every frame too.
The script stops if they differ on even one.

--tb tb/spi_all_tb.v runs the same check with the blob loaded over the SPI port.
"""
import argparse
import os
import shutil
import subprocess
import sys
import time
import zipfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import paths  # noqa: E402

sys.path.insert(0, os.path.join(paths.REPO, "python"))

from dataset import ATTACK_FILES, NORMAL_FILE  # noqa: E402
from forest import hard_vote_predict, known_id_rule  # noqa: E402
from fpga_accuracy import read_attack, read_normal, unpack  # noqa: E402

RTL = [os.path.join("rtl", "engine", f) for f in
       ("sdp_ram.v", "tree_level.v", "rf_engine.v", "rf_classifier.v")] + \
      [os.path.join("rtl", "frontend_can", "can_features.v")]

# The extra files for --spi: the whole core and the flash model.
SPI_RTL = [os.path.join("rtl", "frontend_can", f) for f in ("can_rx.v", "ips_flag.v")] + \
          [os.path.join("rtl", "update", f) for f in
           ("crc32.v", "spi_slave.v", "cmd_decoder.v", "blob_walker.v", "blob_reader.v", "update_core.v")] + \
          [os.path.join("rtl", "nv", f) for f in ("nv_ufm_avmm.v", "boot_rom.v")] + \
          [os.path.join("rtl", "security", f) for f in
           ("aes_sbox.v", "aes128_enc.v", "cmac_stream.v", "ctr_stream.v")] + \
          [os.path.join("rtl", "top", "updatable_core.v"), os.path.join("tb", "ufm_model.v")]


def extract_normal(fname):
    """normal_run_data.txt is only inside the dataset zip: copy it into build/data once."""
    path = os.path.join(paths.BUILD, "data", fname)
    if not os.path.exists(path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        zip_path = paths.find_shared("docs", "refrences", "9) Car-Hacking Dataset.zip")
        print("  extracting %s from the dataset zip" % fname)
        with zipfile.ZipFile(zip_path) as z, z.open("normal_run_data/" + fname) as src, open(path, "wb") as dst:
            shutil.copyfileobj(src, dst)
    return path


def compile_tb(tb, extra, out):
    files = [os.path.join(paths.UPDATABLE, tb)] + [os.path.join(paths.UPDATABLE, f) for f in RTL + extra]
    subprocess.run([paths.IVERILOG, "-g2005", "-I", os.path.join(paths.UPDATABLE, "tb"), "-o", out] + files,
                   check=True)


def run_chunks(sim, blob_hex, ukeys, work, jobs):
    n = len(ukeys)
    bounds = np.linspace(0, n, jobs + 1).astype(int)
    procs, running = [], []
    for c in range(jobs):
        name = "chunk%02d" % c
        with open(os.path.join(work, name + ".hex"), "w") as f:
            f.write("".join("%016X%04X\n" % (h, l) for h, l in ukeys[bounds[c]:bounds[c + 1]].tolist()))
        p = subprocess.Popen([paths.VVP, "-n", sim, "+blob=" + blob_hex, "+in=" + name + ".hex",
                              "+out=" + name + ".out"], cwd=work, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True)
        procs.append(name)
        running.append((name, p))
    t0 = time.time()
    while running:
        time.sleep(1)
        for name, p in list(running):
            if p.poll() is not None:
                out = p.stdout.read()
                if p.returncode != 0 or "DONE" not in out:
                    raise SystemExit("Simulation %s failed:\n%s" % (name, out))
                running.remove((name, p))
    print("  %d chunks simulated in %.0f s" % (jobs, time.time() - t0))
    forest, final = [], []
    for c, name in enumerate(procs):
        with open(os.path.join(work, name + ".out")) as f:
            lines = f.read().split()
        if len(lines) != bounds[c + 1] - bounds[c]:
            raise SystemExit("%s: %d answers for %d frames" % (name, len(lines), bounds[c + 1] - bounds[c]))
        forest += [int(s[0]) for s in lines]
        final += [int(s[1]) for s in lines]
        os.remove(os.path.join(work, name + ".hex"))
        os.remove(os.path.join(work, name + ".out"))
    return np.array(forest, dtype=np.int8), np.array(final, dtype=np.int8)


def check_model(nb, args, sim):
    import joblib
    t0 = time.time()
    name = "rf5_d9_p%d_time" % nb
    bundle = joblib.load(paths.find_shared("models", name + ".joblib"))
    if int(bundle["payload_bytes"]) != nb:
        raise SystemExit("%s reads %s bytes" % (name, bundle["payload_bytes"]))
    data = paths.find_shared("data", "car_hacking")
    keys = []
    for fname, cls in list(ATTACK_FILES.items()) + [(NORMAL_FILE, None)]:
        path = os.path.join(data, fname)
        if not os.path.exists(path):           # normal_run_data.txt: extracted from the dataset zip
            path = extract_normal(fname)
        k = read_attack(path, cls, args.limit, nb)[0] if cls else read_normal(path, args.limit or None, nb)[0]
        keys.append(k)
    ukeys = np.unique(np.concatenate(keys), axis=0)
    total = sum(len(k) for k in keys)
    print("%s: %d frames, %d distinct" % (name, total, len(ukeys)))

    Xu = unpack(ukeys)
    py_forest = hard_vote_predict(bundle["model"], Xu).astype(np.int8)
    py_final = known_id_rule(py_forest, Xu[:, 0].astype(int), bundle["known_ids"]).astype(np.int8)

    work = os.path.join(paths.BUILD, "accuracy", name + ("_enc" if args.enc else ""))
    os.makedirs(work, exist_ok=True)
    blob = os.path.join(paths.BUILD, "test_blobs", "ds_p%d%s.hex" % (nb, "_enc" if args.enc else "")) \
        if args.spi else \
        os.path.join(paths.BUILD, "blobs", "p%d" % nb, "model.hex")
    blob_hex = os.path.relpath(blob, work)
    forest, final = run_chunks(sim, blob_hex.replace("\\", "/"), ukeys, work, args.jobs)
    bad_f, bad_a = int((forest != py_forest).sum()), int((final != py_final).sum())
    print("  engine vs Python on %d distinct frames: forest differs on %d, final on %d (%.0f s)"
          % (len(ukeys), bad_f, bad_a, time.time() - t0))
    if bad_f or bad_a:
        i = np.flatnonzero((forest != py_forest) | (final != py_final))[:5]
        for j in i:
            print("   e.g. %016X%04X engine %d/%d python %d/%d" % (
                ukeys[j][0], ukeys[j][1], forest[j], final[j], py_forest[j], py_final[j]))
        raise SystemExit(1)
    return len(ukeys)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bytes", type=int, nargs="+", default=[5, 6, 8])
    ap.add_argument("--tb", default=os.path.join("tb", "engine_all_tb.v"))
    ap.add_argument("--spi", action="store_true",
                    help="load the blob over the SPI port and COMMIT (tb/spi_all_tb.v)")
    ap.add_argument("--enc", action="store_true",
                    help="with --spi: upload the scrambled blob (ds_pN_enc), so the classifier "
                         "runs on a rulebook the FPGA unscrambled itself")
    ap.add_argument("--extra", nargs="*", default=[], help="more Verilog files, relative to updatable fpgas")
    ap.add_argument("--limit", type=int, default=0, help="frames per file, 0 = all")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    args = ap.parse_args()

    os.makedirs(paths.BUILD, exist_ok=True)
    sim = os.path.join(paths.BUILD, "accuracy_spi_sim" if args.spi else "accuracy_sim")
    if args.enc and not args.spi:
        ap.error("--enc needs --spi")
    if args.spi:
        args.tb, args.extra = os.path.join("tb", "spi_all_tb.v"), SPI_RTL
    compile_tb(args.tb, args.extra, sim)
    for nb in args.bytes:
        check_model(nb, args, sim)
    print("PASS: bit exact with Python for", ", ".join("%d byte" % b for b in args.bytes))


if __name__ == "__main__":
    main()
