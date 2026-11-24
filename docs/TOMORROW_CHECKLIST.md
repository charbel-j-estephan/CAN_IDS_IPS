# Tomorrow: FPGA day checklist

Follow `docs\FPGA_TEST_GUIDE.html` for wiring, switches and what to expect.
This page adds the extra steps the new paper needs.
It also shows where each number goes in `paper.tex`.

Orange text in the PDF = a spot you fill.

---

## 1. Build the new design (guide section 3)

* Build `updatable fpgas\quartus\updatable_top.qpf` in Quartus.
* Then open `updatable fpgas\quartus\output_files\updatable_top.fit.summary`.
* Write down:
  * Total logic elements
  * Total registers
  * M9K blocks (in `updatable_top.fit.rpt`, search "M9Ks")
  * Fmax, slow 1200mV 85C (in `updatable_top.sta.rpt`)

**Goes into** Table IX, "FPGA, updatable" column, rows "Resources" and "Maximum clock".
Today's numbers there are from the Nov 13 build.

## 2. Power (replaces guide T8)

* Double click `docs\power\run_power.bat` after the build.
* Wait for the black window to close (about 3 minutes).
* Open `docs\power\upd.pow.summary`. Write down core dynamic and core static.

**Goes into** Table X, row "FPGA, updatable", and the Power paragraph in Section VII E.
Today it says at most 894 + 94.7 mW. The hardcoded row stays the same.

## 3. Bench tests

| Step | Command | SW2 | Paper spot |
|---|---|---|---|
| T1 quick check | `fpga_tests.bat t1` | down | none, just a check |
| T2 seed 1, majority | `fpga_tests.bat t2` | down | Table VIII, "FPGA updatable, majority" |
| T2 seed 2, majority (new) | `fpga_tests.bat t2s2` | down | same row |
| T3 seed 1, strict | `fpga_tests.bat t3` | UP | Table VIII, "FPGA updatable, unanimous" |
| T3 seed 2, strict (new) | `fpga_tests.bat t3s2` | UP | same row |
| T4a false flag, majority | `fpga_tests.bat t4` | down | Table VIII false flag row |
| T4b false flag, strict | `fpga_tests.bat t4` | UP | Table VIII false flag row |
| T5 lost frames | `fpga_tests.bat t5` | down | paragraph after Table VIII |
| T6, T7 live update | guide Phase B | down | Limitations, first bullet |

**Seed 2 matters most.** It is the run where the STM32 with majority vote pushed the sender into bus off.
If the FPGA with SW2 down does the same, that is a strong result for the paper. Write it down, don't rerun it away.

For every run, copy from `runs\<run folder>\summary.json`:

* `by_class`: sent and arrived for each attack class and for Normal
* the sender line: `state` and `tec`
* the attacker line: `busoffs` and `recovered` (the STM32 runs did not log these, so this is new)
* the FPGA counters, if the guide shows them: flags sent

**Table VIII columns** = Attack frames blocked (sent minus arrived, all attack classes) / Genuine frames delivered / Flags / Sender at end.

## 4. Text spots to fill

* **Abstract**, last sentence: attack frames blocked and genuine frames delivered, all FPGA held out runs together.
* **Section VII C**, "FPGA held-out results: to be added": one or two sentences comparing majority and strict.
* **Limitations**, first bullet: remove it if T2 to T7 are done, or keep only what is still missing.
* **Conclusion**, orange sentence: same numbers as the abstract.

## 5. Finish

* In `paper.tex`, change line 18 to `\newcommand{\tbd}[1]{#1}`.
* Compile twice. Check that no orange text is left.
* Delete `quartus_board\can_ids_top.qsf.powbak` and `updatable fpgas\quartus\updatable_top.qsf.powbak` before `git add`.
* Keep the paper out of the public repo.

If you want, send me the run folders tomorrow and I fill the paper for you.
