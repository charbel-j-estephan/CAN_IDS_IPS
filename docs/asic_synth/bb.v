(* blackbox *) module sdp_ram #(parameter DATA_W=36, parameter ADDR_W=8) (input clk, input we, input [ADDR_W-1:0] waddr, input [DATA_W-1:0] wdata, input [ADDR_W-1:0] raddr, output [DATA_W-1:0] rdata); endmodule
(* blackbox *) module boot_rom #(parameter ADDR_W=15, parameter INIT_HEX="x") (input clk, input [ADDR_W-1:0] addr, output [7:0] q); endmodule
