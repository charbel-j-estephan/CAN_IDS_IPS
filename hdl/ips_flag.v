// IPS: destroys the frame on the bus when the classifier calls it an attack.
// The FPGA version of M3 in stm32_ids/src/main.cpp (flag_begin, flag_step,
// allowed).
//
// An attack verdict starts an active error flag: TX dominant for 6 bit times,
// beginning at the next bit boundary. Six equal bits break the stuffing rule,
// so every node throws the frame away and the sender's transmit error counter
// rises by 8; an attacker that keeps sending goes error passive, then bus off.
//
// Rules (as the STM32):
//   - only while `enable` is on (SW0 on the board; off = TX released at once)
//   - verdict != 0 (any attack class, after the unknown ID rule)
//   - not on the allow list: ID 0x7FF with data "CANST" (the replay's start
//     marker), counted in n_allow
//   - only while the frame can still be destroyed (before_eof from can_rx),
//     checked again when the flag starts; too late: the frame stands, n_ilate
//   - never dominant for more than MAX_BITS bit times, whatever happens:
//     a separate counter cuts TX and counts n_abort (should stay 0)
//
// On the FPGA the verdict comes 4 clocks after feat_ready, so the flag always
// starts on the next bit boundary: no late interrupts, no missed compares.
//
// Test: iverilog -g2005 -o build/ips_flag_sim hdl/ips_flag_tb.v hdl/ips_flag.v
module ips_flag #(
    parameter CLK_HZ    = 50_000_000,
    parameter BIT_HZ    = 500_000,
    parameter FLAG_BITS = 6,             // active error flag: 6 dominant bits
    parameter MAX_BITS  = 8              // safety cut
) (
    input  wire        clk,
    input  wire        rst,              // synchronous, active high
    input  wire        enable,           // prevention on
    input  wire        verdict_valid,    // 1 clock: the final class of the frame on the bus
    input  wire [2:0]  verdict,          // 0 Normal, 1 DoS, 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof
    input  wire [10:0] can_id,           // that frame's ID and data (from can_rx)
    input  wire [63:0] data,             // byte 0 in data[63:56]
    input  wire        before_eof,       // from can_rx: a flag now still destroys the frame
    input  wire        bit_edge,         // from can_rx: 1 clock at the start of every bit
    output reg         tx_dominant = 0,  // 1: pull the bus dominant (the top level drives TX low)
    output reg         flag_start = 0,   // 1 clock: a flag goes on the bus
    output reg  [31:0] n_rej   = 0,      // flags sent
    output reg  [31:0] n_ilate = 0,      // attack verdicts too late to destroy the frame
    output reg  [31:0] n_allow = 0,      // attack verdicts on allow-listed frames
    output reg  [31:0] n_abort = 0       // flags cut by the safety limit
);
    localparam TQ        = CLK_HZ / BIT_HZ;          // clocks per bit (100)
    localparam FLAG_CLKS = FLAG_BITS * TQ;           // 600
    localparam MAX_CLKS  = MAX_BITS * TQ;            // 800

    // The allow list, as allowed() in main.cpp.
    wire allowed = (can_id == 11'h7FF) && (data[63:24] == 40'h43_41_4E_53_54);   // "CANST"

    localparam IDLE  = 2'd0,             // TX released, waiting for an attack verdict
               ARMED = 2'd1,             // flag wanted: wait for the next bit boundary
               FLAG  = 2'd2;             // TX dominant, counting FLAG_CLKS

    reg [1:0] state = IDLE;
    reg [$clog2(MAX_CLKS + 1) - 1:0] cnt = 0;        // clocks of this flag
    reg [$clog2(MAX_CLKS + 1) - 1:0] dom = 0;        // clocks TX has been dominant (safety)

    always @(posedge clk) begin
        flag_start <= 1'b0;

        if (rst || !enable) begin
            state       <= IDLE;
            tx_dominant <= 1'b0;          // released at once, as ips_enable(false)
        end else begin
            case (state)
            IDLE: if (verdict_valid && verdict != 3'd0) begin
                if (allowed)
                    n_allow <= n_allow + 1;
                else if (before_eof)
                    state <= ARMED;
                else
                    n_ilate <= n_ilate + 1;
            end

            ARMED: if (bit_edge) begin
                if (before_eof) begin     // still in time: the flag starts on this boundary
                    state       <= FLAG;
                    tx_dominant <= 1'b1;
                    cnt         <= 1;
                    flag_start  <= 1'b1;
                    n_rej       <= n_rej + 1;
                end else begin            // the frame reached its end of frame meanwhile
                    state   <= IDLE;
                    n_ilate <= n_ilate + 1;
                end
            end

            FLAG: begin
                if (cnt == FLAG_CLKS) begin
                    tx_dominant <= 1'b0;  // 6 bits done: release
                    state       <= IDLE;
                end else begin
                    cnt <= cnt + 1'b1;
                end
            end

            default: state <= IDLE;
            endcase
        end

        // Safety cut, independent of the state machine: it comes last, so it
        // wins over anything above.
        if (tx_dominant) begin
            if (dom >= MAX_CLKS - 1) begin
                tx_dominant <= 1'b0;
                state       <= IDLE;
                n_abort     <= n_abort + 1;
                dom         <= 0;
            end else begin
                dom <= dom + 1'b1;
            end
        end else begin
            dom <= 0;
        end
    end
endmodule
