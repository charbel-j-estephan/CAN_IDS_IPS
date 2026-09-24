# References

IEEE style. Entries [1] to [18] keep the numbers from the literature review
([docs/literature_review.pdf](docs/literature_review.pdf)), so citations in the
review and here match. Entries [19] onward are sources the code and README use.

Corrections to the review itself are in
[docs/literature_review_fixes.md](docs/literature_review_fixes.md).

## Literature review

[1] K. Ileri, A. Rakib, and S. Djahel, "MetaCAN: An optimized adaptive hybrid
metaheuristic based intrusion detection system for CAN bus security,"
*Vehicular Communications*, vol. 55, Art. no. 100956, 2025.

[2] C. Miller and C. Valasek, "Remote exploitation of an unaltered passenger
vehicle," presented at Black Hat USA, 2015.

[3] E. Seo, H. M. Song, and H. K. Kim, "GIDS: GAN based intrusion detection
system for in-vehicle network," in *Proc. 16th Annu. Conf. Privacy, Security
and Trust (PST)*, 2018, pp. 1–6, doi: 10.1109/PST.2018.8514157.
Source of the Car Hacking Dataset. Its Table I matches the dataset files.

[4] D. Donadel, K. Balasubramanian, A. Brighente, B. Ramasubramanian,
M. Conti, and R. Poovendran, "CANTXSec: A deterministic intrusion detection
and prevention system for CAN bus monitoring ECU activations," arXiv preprint
arXiv:2505.09384, 2025.

[5] P. F. de Araujo-Filho, A. J. Pinheiro, G. Kaddoum, D. R. Campelo, and
F. L. Soares, "An efficient intrusion prevention system for CAN: Hindering
cyber-attacks with a low-cost platform," *IEEE Access*, vol. 9,
pp. 166855–166869, 2021.
Source of the 86 µs timing budget used in `python/latency.py`.

[6] H. M. Song, H. R. Kim, and H. K. Kim, "Intrusion detection system based on
the analysis of time intervals of CAN messages for in-vehicle network," in
*Proc. Int. Conf. Information Networking (ICOIN)*, 2016, pp. 63–68.

[7] D. Lee, C. Han, and S. Lee, "Hardware design of intrusion detection system
for automotive CAN bus using random forest," in *Proc. Int. Conf.
Electronics, Information, and Communication (ICEIC)*, 2023,
doi: 10.1109/ICEIC57457.2023.10049883.

[8] S. Abbott-McCune and L. A. Shay, "Intrusion prevention system of
automotive network CAN bus," in *Proc. IEEE Int. Carnahan Conf. Security
Technology (ICCST)*, 2016, doi: 10.1109/CCST.2016.7815711.
Built as an FPGA plus Arduino microcontroller prototype.

[9] M. D. Pese, B. Gozubuyuk, E. Andrechek, H. Olufowobi, M. Hamad, and
K. G. Shin, "MichiCAN: Spoofing and denial-of-service protection using
integrated CAN controllers," in *Proc. 55th Annu. IEEE/IFIP Int. Conf.
Dependable Systems and Networks (DSN)*, 2025, pp. 443–456.

[10] S. Khandelwal and S. Shreejith, "A lightweight multi-attack CAN intrusion
detection system on hybrid FPGAs," in *Proc. Int. Conf. Field-Programmable
Logic and Applications (FPL)*, 2022, pp. 425–429.

[11] A. Rangsikunpum, S. Amiri, and L. Ost, "BIDS: An efficient intrusion
detection system for in-vehicle networks using a two-stage binarized neural
network on low-cost FPGA," *Journal of Systems Architecture*, vol. 156,
Art. no. 103285, 2024.

[12] J. Lee, S. Park, S. Shin, H. Im, J. Lee, and S. Lee, "ASIC design for
real-time CAN-bus intrusion detection and prevention system using random
forest," *IEEE Access*, vol. 13, pp. 129856–129869, 2025.

[13] Y. Jang, H. Im, J. Kim, S. Kim, E. Kim, and S. Lee, "Hardware-software
co-optimized lightweight real-time CAN intrusion detection and prevention
system for ECUs," *Electronics*, vol. 15, no. 10, Art. no. 2108, 2026.

[14] R. Kurachi, Y. Matsubara, H. Takada, N. Adachi, Y. Miyashita, and
S. Horihata, "CaCAN: Centralized authentication system in CAN," in *Proc.
14th Int. Conf. Embedded Security in Cars (escar)*, 2014, pp. 1–9.

[15] W. L. Lambert, S. Ghafoor, and S. Hollifield, "Bitrate hopping: An
intrusion prevention technique to secure the CAN bus based distributed
network," in *Proc. 24th Int. Symp. Parallel and Distributed Computing
(ISPDC)*, 2025, doi: 10.1109/ISPDC67428.2025.00027.

[16] Western OC2 Lab, "IDS-ML: Intrusion detection system development using
machine learning algorithms," GitHub repository. [Online]. Available:
https://github.com/Western-OC2-Lab/Intrusion-Detection-System-Using-Machine-Learning
(submodule `Intrusion-Detection-System-Using-Machine-Learning`).
The matching paper is [24].

[17] J. B. Snyder, "FPGA_random_forest: FPGA implementation of SKLearn random
forest," GitHub repository. [Online]. Available:
https://github.com/johnbensnyder/FPGA_random_forest
(submodule `FPGA_random_forest`). Reviewed but not used, see README.

[18] J. Nordby, "emlearn: Machine learning inference engine for
microcontrollers and embedded devices," Zenodo, 2019,
doi: 10.5281/zenodo.2589394. [Online]. Available:
https://github.com/emlearn/emlearn (submodule `emlearn`).
The review credits "M. L. Jonsson". The author is Jon Nordby, see Fix 6 in
[docs/literature_review_fixes.md](docs/literature_review_fixes.md).

## Dataset

[19] H. M. Song, J. Woo, and H. K. Kim, "In-vehicle network intrusion
detection using deep convolutional neural network," *Vehicular
Communications*, vol. 21, Art. no. 100198, 2020.
A later HCRL paper on the same Car Hacking data. [3] is the primary
dataset citation.

[20] Hacking and Countermeasure Research Lab (HCRL), "Car-Hacking Dataset."
[Online]. Available: https://ocslab.hksecurity.net/Datasets/car-hacking-dataset

## Random forest and machine learning

[21] L. Breiman, "Random forests," *Machine Learning*, vol. 45, no. 1,
pp. 5–32, 2001.

[22] F. Pedregosa *et al.*, "Scikit-learn: Machine learning in Python,"
*Journal of Machine Learning Research*, vol. 12, pp. 2825–2830, 2011.

[23] X. Lin, R. D. Blanton, and D. E. Thomas, "Random forest architectures on
FPGA for multiple applications," in *Proc. Great Lakes Symp. VLSI
(GLSVLSI)*, 2017, pp. 415–418.

[24] L. Yang and A. Shami, "IDS-ML: An open source code for intrusion
detection system development using machine learning," *Software Impacts*,
vol. 14, Art. no. 100446, 2022, doi: 10.1016/j.simpa.2022.100446.

[25] L. Yang, A. Moubayed, I. Hamieh, and A. Shami, "Tree-based intelligent
intrusion detection system in Internet of Vehicles," in *Proc. IEEE Global
Communications Conf. (GLOBECOM)*, 2019, pp. 1–6,
doi: 10.1109/GLOBECOM38437.2019.9013892.

[26] L. Yang, A. Moubayed, and A. Shami, "MTH-IDS: A multi-tiered hybrid
intrusion detection system for Internet of Vehicles," *IEEE Internet of
Things Journal*, vol. 9, no. 1, pp. 616–632, Jan. 2022,
doi: 10.1109/JIOT.2021.3084796.

[27] L. Yang, A. Shami, G. Stevens, and S. DeRusett, "LCCDE: A decision-based
ensemble framework for intrusion detection in the Internet of Vehicles," in
*Proc. IEEE Global Communications Conf. (GLOBECOM)*, 2022, pp. 1–6,
doi: 10.1109/GLOBECOM48099.2022.10001280.

## CAN bus

[28] Robert Bosch GmbH, *CAN Specification, Version 2.0*, Stuttgart,
Germany, 1991.

## Hardware and tools

[29] Terasic Inc., *DE10-Lite User Manual*.

[30] Intel Corp., *Intel MAX 10 FPGA Device Datasheet*.

[31] Intel Corp., *Quartus Prime Lite Edition*.

[32] STMicroelectronics, *RM0090 Reference Manual: STM32F405/415,
STM32F407/417, STM32F427/437 and STM32F429/439 advanced Arm-based 32-bit
MCUs*.

[33] Arm Ltd., *ARMv7-M Architecture Reference Manual* (DWT cycle counter).

[34] S. Williams, *Icarus Verilog*. [Online]. Available:
https://steveicarus.github.io/iverilog/

[35] T. Bybell *et al.*, *GTKWave*. [Online]. Available:
https://gtkwave.sourceforge.net

[36] C. Wolf, *Yosys Open SYnthesis Suite*. [Online]. Available:
https://yosyshq.net/yosys/
