// AES-128 encryption (FIPS-197), one block at a time. Only the forward
// direction: the seal (AES-CMAC) and the scrambling (AES-CTR) never decrypt.
//
// One round every 2 clocks, so each clock does a short piece of work:
//   clock A  the 16 state bytes and the 4 bytes of the last round key word go
//            into 20 S-box ROMs (registered: the answer is there next clock)
//   clock B  ShiftRows, MixColumns (not in round 10), the next round key from
//            the S-box answers, AddRoundKey
// The round keys are made one per round alongside, so none are stored.
//
//   start    1 clock with block_in and key: begins a block. A start while busy
//            drops the block in progress: its result never comes out.
//   done     1 clock, 21 clocks after start, with block_out (held until the next)
//
// Byte order as FIPS-197: byte 0 of the block is bits 127:120.
module aes128_enc (
    input  wire         clk,
    input  wire         rst,
    input  wire         start,
    input  wire [127:0] key,
    input  wire [127:0] block_in,
    output reg          busy = 1'b0,
    output reg          done = 1'b0,
    output reg  [127:0] block_out = 128'd0
);
    reg [127:0] st;            // the state
    reg [127:0] rk;            // the round key of the round just finished
    reg [3:0]   round;         // 1..10
    reg         phase;         // 0 = clock A, 1 = clock B

    // ---- S-boxes: 16 for the state, 4 for the key schedule -----------------
    wire [7:0] sq [0:15];
    wire [7:0] kq [0:3];
    genvar g;
    generate
        for (g = 0; g < 16; g = g + 1) begin : sb
            aes_sbox s (.clk(clk), .a(st[127 - 8*g -: 8]), .q(sq[g]));
        end
        for (g = 0; g < 4; g = g + 1) begin : kb
            aes_sbox s (.clk(clk), .a(rk[31 - 8*g -: 8]), .q(kq[g]));
        end
    endgenerate

    // ---- the next round key -------------------------------------------------
    reg [7:0] rcon;
    always @(*) begin
        case (round)
            4'd1:  rcon = 8'h01;  4'd2:  rcon = 8'h02;  4'd3:  rcon = 8'h04;
            4'd4:  rcon = 8'h08;  4'd5:  rcon = 8'h10;  4'd6:  rcon = 8'h20;
            4'd7:  rcon = 8'h40;  4'd8:  rcon = 8'h80;  4'd9:  rcon = 8'h1B;
            default: rcon = 8'h36;
        endcase
    end
    // SubWord(RotWord(w3)) xor Rcon
    wire [31:0] temp = {kq[1] ^ rcon, kq[2], kq[3], kq[0]};
    wire [31:0] w0 = rk[127:96] ^ temp;
    wire [31:0] w1 = rk[95:64]  ^ w0;
    wire [31:0] w2 = rk[63:32]  ^ w1;
    wire [31:0] w3 = rk[31:0]   ^ w2;
    wire [127:0] rk_next = {w0, w1, w2, w3};

    // ---- ShiftRows and MixColumns -------------------------------------------
    function [7:0] xtime;
        input [7:0] x;
        xtime = {x[6:0], 1'b0} ^ (x[7] ? 8'h1B : 8'h00);
    endfunction

    // byte r + 4c of the shifted state is S-box byte r + 4((c + r) mod 4)
    wire [7:0] sr [0:15];
    generate
        for (g = 0; g < 16; g = g + 1) begin : shift
            assign sr[g] = sq[(g % 4) + 4 * (((g / 4) + (g % 4)) % 4)];
        end
    endgenerate

    wire [127:0] after_sr, after_mc;
    generate
        for (g = 0; g < 4; g = g + 1) begin : col
            wire [7:0] a0 = sr[4*g], a1 = sr[4*g+1], a2 = sr[4*g+2], a3 = sr[4*g+3];
            assign after_sr[127 - 32*g -: 32] = {a0, a1, a2, a3};
            assign after_mc[127 - 32*g -: 32] = {
                xtime(a0) ^ xtime(a1) ^ a1 ^ a2 ^ a3,
                a0 ^ xtime(a1) ^ xtime(a2) ^ a2 ^ a3,
                a0 ^ a1 ^ xtime(a2) ^ xtime(a3) ^ a3,
                xtime(a0) ^ a0 ^ a1 ^ a2 ^ xtime(a3)
            };
        end
    endgenerate

    wire [127:0] st_next = ((round == 4'd10) ? after_sr : after_mc) ^ rk_next;

    always @(posedge clk) begin
        done <= 1'b0;
        if (rst) begin
            busy <= 1'b0;
        end else if (start) begin
            st    <= block_in ^ key;           // round 0: AddRoundKey
            rk    <= key;
            round <= 4'd1;
            phase <= 1'b0;
            busy  <= 1'b1;
        end else if (busy) begin
            if (!phase) begin
                phase <= 1'b1;                 // the S-boxes read st and rk now
            end else begin
                phase <= 1'b0;
                st    <= st_next;
                rk    <= rk_next;
                if (round == 4'd10) begin
                    block_out <= st_next;
                    done      <= 1'b1;
                    busy      <= 1'b0;
                end else begin
                    round <= round + 1'b1;
                end
            end
        end
    end
endmodule
