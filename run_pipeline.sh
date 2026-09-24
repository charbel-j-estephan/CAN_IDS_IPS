#!/usr/bin/env bash
# Train, export to Verilog and C, then prove all three agree.
#
#   ./run_pipeline.sh data/car_hacking            real dataset
#   ./run_pipeline.sh data/car_hacking 50 8       50 trees, max depth 8
#   ./run_pipeline.sh demo                        fake data, checks your tools
set -euo pipefail
cd "$(dirname "$0")"

DATA="${1:-demo}"
TREES="${2:-100}"
DEPTH="${3:-10}"
PY="${PYTHON:-python}"
command -v "$PY" >/dev/null || PY=python3

if [ "$DATA" = "demo" ]; then
    "$PY" python/make_demo_data.py --out data/demo
    DATA=data/demo
fi

mkdir -p build
"$PY" python/train.py --data "$DATA" --trees "$TREES" --max-depth "$DEPTH"
"$PY" python/export_verilog.py
"$PY" python/export_c.py

if command -v iverilog >/dev/null; then
    iverilog -g2005 -I hdl/generated -o build/rf_ids_sim hdl/rf_ids_tb.v hdl/generated/rf_ids.v
    vvp -n build/rf_ids_sim | grep -E "PASS|FAIL|mismatch"
else
    echo "Skipping Verilog simulation: install Icarus Verilog (iverilog)"
fi

if command -v gcc >/dev/null; then
    gcc -O2 -I mcu/generated -o build/host_test mcu/host_test.c
    ./build/host_test hdl/generated/vectors.hex
else
    echo "Skipping C check: install gcc"
fi
