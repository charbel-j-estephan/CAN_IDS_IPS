// One byte of CRC-32 (IEEE 802.3, the one Python's zlib.crc32 uses).
// Start with crc = FFFFFFFF; the final value is ~crc.
module crc32_byte (
    input  wire [31:0] crc,
    input  wire [7:0]  d,
    output reg  [31:0] next
);
    integer i;
    always @(*) begin
        next = crc ^ {24'b0, d};
        for (i = 0; i < 8; i = i + 1)
            next = next[0] ? (next >> 1) ^ 32'hEDB88320 : (next >> 1);
    end
endmodule
