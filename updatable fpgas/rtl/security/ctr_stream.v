// AES-CTR keystream, one byte at a time: the scrambling.
//
// Block b of the keystream is AES(K_enc, IV || b), b a 32 bit big endian block
// number from 0, IV the 12 bytes from the blob header (offsets 40-51). Byte k
// of the scrambled region is xored with keystream byte k. Xoring twice gives
// the original back, so the same stream unscrambles (blob_walker, on the way
// in) and scrambles again (blob_reader, on the way to flash).
//
//   restart   1 clock: back to keystream byte 0 with this iv (the walker when
//             the IV is in, the reader at each start). A block still being made
//             for the old stream is dropped (the AES start aborts it).
//   ks_valid  ks_byte is the keystream byte at the current position
//   take      1 clock, only while ks_valid: move on one byte. After the 16th
//             byte of a block the next block is made: ks_valid is 0 for ~21 clocks
module ctr_stream (
    input  wire        clk,
    input  wire [127:0] key,       // a constant in the design (the board key)
    input  wire        rst,
    input  wire        restart,
    input  wire [95:0] iv,
    input  wire        take,
    output reg         ks_valid = 1'b0,
    output wire [7:0]  ks_byte
);
    reg          aes_start = 1'b0;
    reg  [127:0] aes_in;
    wire         aes_busy, aes_done;
    wire [127:0] aes_out;
    aes128_enc aes (.clk(clk), .rst(rst), .start(aes_start), .key(key), .block_in(aes_in),
                    .busy(aes_busy), .done(aes_done), .block_out(aes_out));

    reg  [31:0]  blk = 32'd0;       // the number of the next block to make
    reg  [95:0]  iv_r;
    reg  [127:0] ks;
    reg  [3:0]   pos = 4'd0;
    reg          making = 1'b0;     // the AES is making a block of this stream

    assign ks_byte = ks[127 - 8*pos -: 8];

    always @(posedge clk) begin
        aes_start <= 1'b0;
        if (rst) begin
            ks_valid <= 1'b0;
            making   <= 1'b0;
            blk      <= 32'd0;
            pos      <= 4'd0;
        end else if (restart) begin
            iv_r      <= iv;
            aes_in    <= {iv, 32'd0};
            aes_start <= 1'b1;
            making    <= 1'b1;
            blk       <= 32'd1;
            pos       <= 4'd0;
            ks_valid  <= 1'b0;
        end else begin
            if (making && aes_done) begin
                ks       <= aes_out;
                ks_valid <= 1'b1;
                making   <= 1'b0;
                pos      <= 4'd0;
            end
            if (take && ks_valid) begin
                if (pos == 4'd15) begin
                    ks_valid  <= 1'b0;
                    aes_in    <= {iv_r, blk};
                    aes_start <= 1'b1;
                    making    <= 1'b1;
                    blk       <= blk + 1'b1;
                end else begin
                    pos <= pos + 1'b1;
                end
            end
        end
    end
endmodule
