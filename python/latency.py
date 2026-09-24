"""Turn the Quartus Fmax into per frame latency and compare it to your budget.

Example, after Quartus reports Fmax = 142.3 MHz:
    python python/latency.py --fmax 142.3

Find Fmax in Quartus under Compilation Report, Timing Analyzer,
Slow 1200mV 85C Model, Fmax Summary. Use the slow model number, it is the
worst case the chip is guaranteed to meet.

The budget follows Araujo Filho et al. (IEEE Access, 2021). A detector that
decides after the first N of 8 payload bytes has 19 + 8 * (8 - N) bit times
left before the end of frame field, the last chance to force an error frame.
At 500 kbit/s that is 86 us for N = 5 and 38 us for N = 8. Train with
--payload-bytes N to match.
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
    ap.add_argument("--payload-bytes", type=int, default=None, metavar="N",
                    help="payload bytes the model reads, default: from --model")
    ap.add_argument("--model", default="models/can_ids_forest.joblib")
    ap.add_argument("--bitrate-kbps", type=float, default=500.0)
    ap.add_argument("--budget-us", type=float, default=None,
                    help="override the budget computed from --payload-bytes")
    args = ap.parse_args()

    if args.budget_us is None:
        n = args.payload_bytes
        if n is None:
            try:
                import joblib
                n = joblib.load(args.model).get("payload_bytes", 8)
            except (OSError, ImportError):
                n = 8
        bits = 19 + 8 * (8 - n)
        args.budget_us = bits * 1000.0 / args.bitrate_kbps
        print("Budget: %d payload bytes read, %d bit times left at %.0f kbit/s = %.1f us"
              % (n, bits, args.bitrate_kbps, args.budget_us))

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
