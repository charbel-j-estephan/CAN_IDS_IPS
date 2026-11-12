// AES-CMAC (RFC 4493) over a byte stream: the seal check.
//
// The rulebook streams in one byte at a time (from SPI, flash or the ROM); this
// keeps a running CMAC and has the tag ready soon after the last message byte.
// The caller compares the tag with the MAC stored in the rulebook.
//
//   restart    1 clock: a new message begins (the walker's start)
//   msg_len    message length in bytes, used once len_valid is 1. For a blob:
//              total_len - 16 (everything before the MAC, the CRC included)
//   len_valid  msg_len can be trusted (the blob header's length field is in)
//   in_valid   1 clock: in_byte is taken (only when ready was 1)
//   ready      a byte may be taken now. 0 only while the subkeys are made
//              after reset, or when this byte would start an AES block while
//              the previous one is still running
//   tag        the CMAC, byte 0 in bits 127:120, valid while tag_done = 1
//   tag_strobe 1 clock when tag becomes valid
//
// Bytes after the message (the MAC itself) are taken and ignored. A message
// whose length never matches (0, or a header that lies) never gets a tag, so
// the seal check fails instead of hanging. RFC 4493's 0 byte example therefore
// has no tag here; a blob is never shorter than 708 bytes.
module cmac_stream (
    input  wire         clk,
    input  wire [127:0] key,       // a constant in the design (the board key)
    input  wire         rst,
    input  wire         restart,
    input  wire [15:0]  msg_len,
    input  wire         len_valid,
    input  wire         in_valid,
    input  wire [7:0]   in_byte,
    output wire         ready,
    output reg  [127:0] tag = 128'd0,
    output reg          tag_done = 1'b0,
    output reg          tag_strobe = 1'b0
);
    reg          aes_start = 1'b0;
    reg  [127:0] aes_in;
    wire         aes_busy, aes_done;
    wire [127:0] aes_out;
    aes128_enc aes (.clk(clk), .rst(rst), .start(aes_start), .key(key), .block_in(aes_in),
                    .busy(aes_busy), .done(aes_done), .block_out(aes_out));

    // ---- subkeys K1, K2 from L = AES(K, 0), once after reset ----------------
    reg          sk_ok  = 1'b0;     // K1 and K2 are ready
    reg          sk_run = 1'b0;     // the AES is making L
    reg  [127:0] k1, k2;
    wire [127:0] l_dbl  = {aes_out[126:0], 1'b0} ^ (aes_out[127] ? 128'h87 : 128'h0);
    wire [127:0] l_dbl2 = {l_dbl[126:0], 1'b0}   ^ (l_dbl[127]   ? 128'h87 : 128'h0);

    // ---- the message --------------------------------------------------------
    reg  [15:0]  cnt  = 16'd0;      // message bytes taken
    reg  [3:0]   fill = 4'd0;       // bytes in buf (0..15)
    reg  [127:0] buf_;              // the block being collected, byte 0 at 127:120
    reg  [127:0] x    = 128'd0;     // the CBC chain
    reg          pending = 1'b0;    // an AES block of this message is running
    reg          final_ = 1'b0;     // ... and it is the last one
    reg          ended = 1'b0;      // the last block has gone in

    wire in_msg   = !ended;
    wire last     = len_valid && (cnt + 16'd1 == msg_len);
    wire triggers = in_msg && (fill == 4'd15 || last);     // this byte starts an AES block
    assign ready  = sk_ok && !(triggers && pending);

    // The block with this byte in it, and the padded / subkeyed last block.
    wire [127:0] with_byte = buf_ | ({120'd0, in_byte} << (8 * (15 - fill)));
    wire [127:0] pad       = (fill == 4'd15) ? 128'd0 : (128'h80 << (8 * (14 - fill)));
    wire [127:0] last_blk  = (fill == 4'd15) ? (with_byte ^ k1) : (with_byte ^ pad ^ k2);

    always @(posedge clk) begin
        aes_start  <= 1'b0;
        tag_strobe <= 1'b0;
        if (rst) begin
            sk_ok    <= 1'b0;
            sk_run   <= 1'b1;
            aes_in   <= 128'd0;
            aes_start <= 1'b1;               // L = AES(K, 0)
            cnt      <= 16'd0;
            fill     <= 4'd0;
            buf_     <= 128'd0;
            x        <= 128'd0;
            pending  <= 1'b0;
            final_   <= 1'b0;
            ended    <= 1'b0;
            tag_done <= 1'b0;
        end else begin
            if (sk_run && aes_done) begin
                k1     <= l_dbl;
                k2     <= l_dbl2;
                sk_ok  <= 1'b1;
                sk_run <= 1'b0;
            end

            if (restart) begin
                // Forget the old message. An AES block of it that is still
                // running is dropped: the next start aborts it, and `pending`
                // is cleared so its done is not taken as this message's.
                cnt      <= 16'd0;
                fill     <= 4'd0;
                buf_     <= 128'd0;
                x        <= 128'd0;
                pending  <= 1'b0;
                final_   <= 1'b0;
                ended    <= 1'b0;
                tag_done <= 1'b0;
            end else begin
                if (pending && aes_done && !sk_run) begin
                    pending <= 1'b0;
                    if (final_) begin
                        tag        <= aes_out;
                        tag_done   <= 1'b1;
                        tag_strobe <= 1'b1;
                    end else begin
                        x <= aes_out;
                    end
                end

                if (in_valid && in_msg) begin
                    cnt <= cnt + 1'b1;
                    if (last) begin
                        aes_in    <= x ^ last_blk;
                        aes_start <= 1'b1;
                        pending   <= 1'b1;
                        final_    <= 1'b1;
                        ended     <= 1'b1;
                        buf_      <= 128'd0;
                        fill      <= 4'd0;
                    end else if (fill == 4'd15) begin
                        aes_in    <= x ^ with_byte;
                        aes_start <= 1'b1;
                        pending   <= 1'b1;
                        final_    <= 1'b0;
                        buf_      <= 128'd0;
                        fill      <= 4'd0;
                    end else begin
                        buf_ <= with_byte;
                        fill <= fill + 1'b1;
                    end
                end
            end
        end
    end
endmodule
