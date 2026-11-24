# ASIC timing and power with OpenSTA (23 Nov 2026)

SkyWater 130 nm HD, typical corner (tt_025C_1v80), after synthesis only. Memories are black boxes (mem_bb.lib, no timing, no power).

| Block | Area | Critical path | Logic power at 50 MHz |
|---|---|---|---|
| Hardcoded forest (rf_ids) | 0.010 mm2 | 2.86 ns | 0.19 mW |
| Whole updatable core | 0.688 mm2 | 7.73 ns (about 129 MHz) | 21.4 mW |

Timing. Netlists resynthesized with ABC buffering and sizing (netb_*.ys, abc_buf.script). Nets with more than 32 loads get 0.35 ns driver delay plus 1.0 ns buffer tree delay (hfanno.py writes the set_assigned_delay commands). Ideal clock, no wires. Without this the core shows 33 ns because one flop drives 1,919 loads.

Power. pow_*.tcl with read_vcd. VCDs from the same 20 ms Icarus simulations as the Quartus run, cleaned (cleanvcd.py) and, for the core, flattened to match the flat netlist (flatvcd.py). About 30 percent of core pins annotated from the VCD, the rest propagated. No SRAM, clock tree or wires. Almost all of the 21.4 mW is flop internal power (about 9,900 flops clocked every cycle).

OpenSTA 3.2.0 built from github.com/The-OpenROAD-Project/OpenSTA with CUDD.
