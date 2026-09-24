# 50 MHz board clock. Fmax in the Timing Analyzer report tells you how far
# above or below this the design can run.
create_clock -name clk -period 20.000 [get_ports clk]
derive_clock_uncertainty
