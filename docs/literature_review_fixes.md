# Literature review fixes

Drop in replacements for `docs/literature_review.pdf`. Each fix quotes the
current text, gives the replacement, and says why. The replacement text
follows the review's own style, with no hyphens or dashes.

Fixes 1 to 6 correct errors. Fix 7 adds a missing point about the closest
prior design. Fix 8 replaces reference entries.

## Fix 1. Section 3.1, Lee et al. do not use fixed thresholds

**Current:**
> Lee et al. built on this idea with a set of fixed thresholds, comparing each
> message against its identifier's average interval and payload Hamming
> distance to flag spoofing [7].

**Replace with:**
> Lee et al. built on this idea by measuring how far each frame's interval and
> payload Hamming distance deviate from the averages recorded for its
> identifier in normal traffic, then passing those two deviations to a three
> tree random forest trained in WEKA to flag spoofing [7].

**Why:** the paper trains a random forest on these two features (Sections
II.C and III.A of [7]). No fixed threshold decides the class.

## Fix 2. Section 3.2, MetaCAN does not train only on normal traffic

**Current:**
> Two of the reviewed designs train only on normal traffic, which lets them
> flag attacks without ever seeing a labeled example.

**Replace with:**
> Some of the reviewed designs train only on normal traffic, which lets them
> flag attacks without ever seeing a labeled example, while others train on
> labeled attacks.

**Why:** the paragraph then describes Araujo Filho et al. (trained on normal
traffic) and MetaCAN, a supervised XGBoost classifier trained on labeled
attacks. GIDS's second discriminator is the other normal only design, and it
is already covered one paragraph earlier.

## Fix 3. Section 3.2, XGBoost trees do not run in sequence at inference

**Current:**
> That independence is what makes a random forest a better fit for a hardware
> implementation, since every tree can be evaluated in parallel with no
> dependency on the others, whereas XGBoost's trees must be evaluated in
> sequence.

**Replace with:**
> The dependence between XGBoost trees exists only during training. Once
> trained, both models can evaluate every tree in parallel, since a random
> forest combines its trees by a vote and XGBoost combines them by a sum. The
> difference that matters for hardware is in that last step. Each random
> forest leaf holds a class label, so combining the trees needs only a small
> vote counter, while each XGBoost leaf holds a real valued score, so
> combining them needs an adder tree and a final comparison.

**Why:** XGBoost builds trees in sequence, each one correcting the last, but a
trained XGBoost model adds up independent tree outputs. Every tree reads the
same input and none needs another tree's result.

## Fix 4. Section 4, the FPGA designs are not in the single digit microsecond range

**Current:**
> Once a classifier is committed to dedicated hardware, whether an FPGA or an
> application specific circuit, detection consistently lands in the single
> digit microsecond range, well inside the timing budget the protocol allows,
> so the open question is no longer whether hardware detection is fast enough
> but which platform delivers that speed at the lowest cost and power for a
> given accuracy target.

**Replace with:**
> Once a classifier is committed to an application specific circuit,
> detection lands in the single digit microsecond range, well inside the
> timing budget the protocol allows. The FPGA designs in Table 1 took 0.169
> and 0.43 milliseconds per decision, both above the 86 microsecond budget
> from Section 3.3, but both ran neural networks. So the open question is
> whether a lighter, tree based model can bring an FPGA or a microcontroller
> inside that budget, and which platform then delivers that speed at the
> lowest cost and power for a given accuracy target.

**Why:** Table 1 lists 0.43 ms for Khandelwal and Shreejith [10] and
0.169 ms for Rangsikunpum et al. [11]. Only the two ASICs [12, 13] reach
single digit microseconds.

## Fix 5. Section 4, the three tools paragraph

The project could not use two of the three tools as the review describes
them. See the README section "Why not use the upstream repos directly".

**Current:** the paragraph starting "Three open source tools make this three
way comparison practical" through "without a separate hand written
implementation for each one."

**Replace with:**
> Three open source projects shaped the toolchain for this three way
> comparison. The IDS ML repository supplies tree based training code for
> vehicle intrusion detection [16]. Its notebooks load the CICIDS2017 dataset,
> so this project wrote its own loader and training script for the Car Hacking
> Dataset. The FPGA random forest project generates Verilog from a trained
> scikit learn forest [17], but it averages one class's leaf values instead of
> voting on a class label, and it no longer runs on current scikit learn
> releases. This project therefore wrote its own generator, which produces a
> pipelined voting classifier that accepts one frame per clock. The emlearn
> library converts the same forest into portable C for the microcontroller
> [18], after a small adjustment to the split thresholds so that its
> comparisons match scikit learn exactly. A shared set of test frames confirms
> that the FPGA logic, the C code, and the Python model return the same class
> for every frame, so all three builds run exactly one model.

## Fix 6. Reference [18], wrong author

emlearn's author is Jon Nordby, not "M. L. Jonsson". Use the entry in Fix 8.

## Fix 7. Add Abbott McCune and Shay's FPGA plus microcontroller prototype

The review describes their ownership rule in Section 3.1 but leaves out their
hardware. They ran the rule as VHDL on a small Altera FPGA board that watches
the bus, paired with an Arduino Uno microcontroller for decisions, one of each
per node, and verified it in ModelSim (Section IV.B and Table III of [8]).
That is the closest prior design to this project's combined build, so a
reviewer will expect it.

**Add to Section 3.3, after the paragraph on Jang et al.:**
> Abbott McCune and Shay had already split a CAN defense across the same two
> kinds of device this project compares. Their prototype runs the ownership
> rule from Section 3.1 as VHDL on a small Altera FPGA board that follows the
> receive and transmit lines bit by bit, pairs it with an Arduino Uno
> microcontroller that makes decisions for the node, and lets the node force
> a spoofed frame to error out [8]. They verify the design in ModelSim and do
> not report latency or power, and because the rule is fixed rather than
> learned, the design gives no information about how a trained classifier
> behaves on the same split.

**Add a row to Table 1:**

| Study | Platform | Model | Latency | Power or Energy |
|---|---|---|---|---|
| Abbott McCune and Shay [8] | Altera FPGA plus Arduino Uno | Identifier ownership rule | Not reported | Not reported |

**Add to Section 3.4, after "...where the timing budget breaks down [4,9].":**
> Abbott McCune and Shay pair an FPGA with a microcontroller, but they use the
> pair as a single design with a fixed rule, and they measure neither device on
> its own [8].

## Fix 8. Replacement reference entries

These follow the review's reference style. The details come from the PDFs of
the papers.

> [3] E. Seo, H. M. Song, and H. K. Kim, GIDS, GAN based intrusion detection
> system for in vehicle network, in Proceedings of the 16th Annual Conference
> on Privacy, Security and Trust, 2018, pages 1 to 6, doi
> 10.1109/PST.2018.8514157.

> [7] D. Lee, C. Han, and S. Lee, Hardware design of intrusion detection
> system for automotive CAN bus using random forest, in Proceedings of the
> International Conference on Electronics, Information, and Communication,
> 2023, doi 10.1109/ICEIC57457.2023.10049883.

> [8] S. Abbott McCune and L. A. Shay, Intrusion prevention system of
> automotive network CAN bus, in Proceedings of the IEEE International
> Carnahan Conference on Security Technology, 2016, doi
> 10.1109/CCST.2016.7815711.

> [15] W. L. Lambert, S. Ghafoor, and S. Hollifield, Bitrate hopping, an
> intrusion prevention technique to secure the CAN bus based distributed
> network, in Proceedings of the 24th International Symposium on Parallel and
> Distributed Computing, 2025, doi 10.1109/ISPDC67428.2025.00027.

> [18] J. Nordby, emlearn, machine learning inference engine for
> microcontrollers and embedded devices, Zenodo, 2019, doi
> 10.5281/zenodo.2589394, github.com/emlearn/emlearn.

The [3] DOI comes from the paper's IEEE article number (8514157) and follows
the same pattern as the others. Open it once to confirm.

## Checked and correct

* [3] is the right citation for the Car Hacking Dataset. GIDS Table I lists
  the same message counts as the dataset files (for example 3,665,771 frames
  in the DoS set), recorded on the same YF Sonata.
* [7] reports 98.2 percent accuracy with three trees, and its voter takes two
  of three, as the review says.
* [8] matches the review's description of the ownership rule. A node flags a
  frame whose identifier it owns while it is not transmitting.
* [15] matches the review's description of bitrate hopping, and the design
  needs every trusted node to follow the same hop sequence.
