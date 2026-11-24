read_liberty hd_tt.lib
read_liberty mem_bb.lib
read_verilog net_core_sta.v
link_design updatable_core
create_clock -name clk -period 20 [get_ports clk]
set_input_delay 0 -clock clk [delete_from_list [all_inputs] [get_ports clk]]
read_vcd -scope updatable_top/core /home/claude/pow/upd/upd_flat.vcd
report_activity_annotation
report_power
report_power -instances [get_cells -quiet {classifier.*}] -digits 3
