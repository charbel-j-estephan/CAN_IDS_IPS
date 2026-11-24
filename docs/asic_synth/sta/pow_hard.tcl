read_liberty hd_tt.lib
read_verilog netb_hard.v
link_design rf_ids
create_clock -name clk -period 20 [get_ports clk]
set_input_delay 0 -clock clk [delete_from_list [all_inputs] [get_ports clk]]
read_vcd -scope frames_tb/forest /home/claude/pow/hard/hard_clean.vcd
report_activity_annotation
report_power
