// The baked-in default model: a blob in ROM, loaded through the same path as
// one from flash. On the MAX 10 this needs a "with Memory Initialization"
// configuration mode (checked in Step 0: without it Quartus puts the ROM in
// logic). On an ASIC this becomes a real ROM macro.
module boot_rom #(
    parameter ADDR_W   = 13,                 // 6144 bytes used
    parameter INIT_HEX = "default_model.hex"
) (
    input  wire              clk,
    input  wire [ADDR_W-1:0] addr,
    output reg  [7:0]        q
);
    (* ram_init_file = "default_model.mif" *) reg [7:0] rom [0:6143];
    // synthesis translate_off
    initial $readmemh(INIT_HEX, rom);
    // synthesis translate_on
    always @(posedge clk) q <= rom[addr];
endmodule
