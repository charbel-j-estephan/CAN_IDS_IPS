read_liberty hd_tt.lib
read_verilog netb_hard.v
link_design rf_ids
create_clock -name clk -period 20 [get_ports clk]
set_input_delay 0 -clock clk [delete_from_list [all_inputs] [get_ports clk]]
set_output_delay 0 -clock clk [all_outputs]
set_load 0.01 [all_outputs]
source hf_hard.tcl
report_checks -path_delay max -fields {} -digits 3 
report_wns
report_tns
report_clock_min_period
