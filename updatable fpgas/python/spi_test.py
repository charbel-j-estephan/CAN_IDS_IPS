"""Measure the SPI link between the STM32 update tool and the FPGA.

    py -3.13 spi_test.py COM11            the standard set below, 1000 reads each
    py -3.13 spi_test.py COM11 5000       5000 reads each

Each line is 1000 STATUS reads at one clock speed and edge speed; `bad` counts
the reads that did not start with C1. The first line is the normal setting and
must show bad=0. Edge: 0 slow (normal), 1 medium, 2 fast, 3 very fast (the
core's default, which lost about 4 reads in 10 on jumper wires).
Needs the updater build that answers ID with "updater 3" or later.
"""
import sys
import time

import serial

RUNS = [            # (kHz, edge)
    (2625, 0),      # the normal setting since updater 3: must be bad=0
    (1312, 0),
    (2625, 1),      # faster edges, for comparison: expect losses on jumper wires
]


def ask(port, line):
    port.write((line + "\n").encode())
    return port.readline().decode(errors="replace").strip()


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
    port = serial.Serial(sys.argv[1], 115200, timeout=20)
    time.sleep(0.5)
    port.reset_input_buffer()
    ident = ask(port, "ID")
    print(ident or "no answer to ID")
    if not ident.startswith("ID updater"):
        raise SystemExit("This is the old updater build: flash the new one first.")
    for khz, edge in RUNS:
        print(ask(port, "TEST %d %d %d" % (n, khz, edge)))
    port.close()


if __name__ == "__main__":
    main()
