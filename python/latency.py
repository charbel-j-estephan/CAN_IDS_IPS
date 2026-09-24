"""Turn the Quartus Fmax into per frame latency and compare it to your budget.

Example, after Quartus reports Fmax = 142.3 MHz:
    python python/latency.py --fmax 142.3

Find Fmax in Quartus under Compilation Report, Timing Analyzer,
Slow 1200mV 85C Model, Fmax Summary. Use the slow model number, it is the
worst case the chip is guaranteed to meet.
"""
import argparse

from export_verilog import LATENCY


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fmax", type=float, required=True, help="Fmax in MHz")
    ap.add_argument("--clock", type=float, default=50.0,
                    help="clock you will actually run, MHz (DE10 Lite: 50)")
    ap.add_argument("--cycles", type=int, default=LATENCY,
                    help="clocks per classification (rf_ids: %d)" % LATENCY)
    ap.add_argument("--budget-us", type=float, default=86.0)
    args = ap.parse_args()

    for label, mhz in (("at Fmax", args.fmax), ("at your clock", min(args.clock, args.fmax))):
        us = args.cycles / mhz
        verdict = "UNDER" if us <= args.budget_us else "OVER"
        print("%-14s %7.2f MHz  %d cycles  %8.4f us  %s the %.1f us budget"
              % (label, mhz, args.cycles, us, verdict, args.budget_us))
    if args.clock > args.fmax:
        print("Warning: %.1f MHz is above Fmax. The design fails timing at that clock."
              % args.clock)
    print("Throughput: one new frame every clock, %.1f million frames per second at %.1f MHz"
          % (min(args.clock, args.fmax), min(args.clock, args.fmax)))


if __name__ == "__main__":
    main()
