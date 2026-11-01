// CAN receiver for the DE10 Lite: turns the transceiver's RXD into frames.
// The FPGA version of stm32_ids/src/can_rx.c and the TIM4 bit timing in
// main.cpp. Standard data frames at 500 kbit/s from a 50 MHz clock.
//
// Three parts, all running at the same time:
//   1. input stage: synchronizer + falling edge detector (fall_detect.v)
//   2. bit timing:  counter 0..99 (one bit), restarted by every falling edge
//                   (hard sync / resync), sample pulse at 75 % of the bit
//   3. frame state machine, one step per sample: WAIT_IDLE, IDLE, IN_FRAME,
//      TAIL, the same states and rules as can_rx.c
//
// Tests (from the repo root):
//   iverilog -g2005 -I hdl/vectors -o build/can_rx_sim hdl/can_rx_tb.v hdl/can_rx.v
//   vvp build/can_rx_sim            (+drift_ppm=3000, +drift_ppm=-3000)
//   and the dataset frames: hdl/frames_tb.v, python/compare_sim.py
module can_rx #(
    parameter CLK_HZ     = 50_000_000,
    parameter BIT_HZ     = 500_000,
    parameter SAMPLE_PCT = 75,           // sample point, % of the bit after its start
    parameter FEAT_BYTES = 5             // data bytes the model reads: feat_ready comes
                                         // after this many (5 = the STM32's model)
) (
    input  wire        clk,
    input  wire        rst,              // synchronous, active high
    input  wire        rxd,              // from the transceiver: 1 recessive, 0 dominant
    output reg         frame_valid,      // 1 clock: good frame, at the 6th end of frame bit
    output reg  [10:0] can_id,
    output reg  [3:0]  dlc,              // as sent, 0..15
    output reg  [63:0] data,             // byte 0 in data[63:56], bytes past the DLC are 0
    output reg         crc_err,          // 1 clock: CRC wrong
    output reg         stuff_err,        // 1 clock: 6 equal bits in a row
    output reg         form_err,         // 1 clock: a fixed recessive bit was dominant
    output reg         id_ready,         // 1 clock: the 11 ID bits are in
    output reg         dlc_ready,        // 1 clock: the 4 DLC bits are in
    output reg         feat_ready,       // 1 clock: ID, DLC and the first FEAT_BYTES data bytes
                                         // are in (all of them if the DLC is smaller)
    output wire        before_eof,       // 1: an error flag started now still destroys the frame
    output reg         bit_edge          // 1 clock: a new bit time starts (for the IPS flag)
);
    // ---- 1. input stage ------------------------------------------------
    // Two flip-flops make rxd safe to use (metastability); the third holds the
    // previous value, so "was 1, is 0" is a falling edge. Starts recessive.
    reg [2:0] s = 3'b111;
    always @(posedge clk) s <= {s[1:0], rxd};
    wire rx   = s[1];
    wire fall = s[2] & ~s[1];

    // ---- 2. bit timing -------------------------------------------------
    // TQ clocks per bit (100). Any falling edge means "a bit starts here", so
    // the counter restarts: at a start of frame that is the hard sync, inside a
    // frame the resync that keeps us in step with the sender's clock.
    localparam TQ = CLK_HZ / BIT_HZ;
    localparam SP = TQ * SAMPLE_PCT / 100;

    reg [$clog2(TQ)-1:0] cnt = 0;
    reg sample = 0;                       // 1 clock: lvl was just updated
    reg lvl    = 1;                       // the bus level at the last sample point

    always @(posedge clk) begin
        sample   <= 1'b0;
        bit_edge <= 1'b0;
        if (fall) begin
            cnt      <= 1;                // this clock is count 0 of the new bit
            bit_edge <= 1'b1;
        end else if (cnt == TQ - 1) begin
            cnt      <= 0;
            bit_edge <= 1'b1;
        end else begin
            cnt <= cnt + 1'b1;
        end
        if (cnt == SP) begin
            sample <= 1'b1;
            lvl    <= rx;
        end
    end

    // ---- 3. frame state machine ---------------------------------------
    localparam WAIT_IDLE = 2'd0,          // after reset or an error: wait for recessive bits
               IDLE      = 2'd1,          // bus idle: a dominant bit is a start of frame
               IN_FRAME  = 2'd2,          // SOF to the end of the CRC, stuffed
               TAIL      = 2'd3;          // CRC delimiter, ACK, end of frame (not stuffed)

    reg [1:0]  state = WAIT_IDLE;
    reg [6:0]  nbits;                     // bits after destuffing, SOF = bit 0 (max 98)
    reg [6:0]  frame_end;                 // nbits when the CRC is complete
    reg [6:0]  feat_end;                  // nbits when the features are complete
    reg [14:0] crc;                       // CRC-15, 0 at the end of a good frame
    reg        last;                      // previous bit on the wire
    reg [2:0]  run;                       // equal bits in a row, for destuffing
    reg        stuff_next;                // the next bit on the wire is a stuff bit
    reg        rtr;                       // remote frame: no data bytes
    reg [3:0]  tail;                      // position in the TAIL state
    reg [3:0]  rec_run = 0;               // recessive bits in a row (WAIT_IDLE)
    reg [3:0]  idle_need = 11;            // 11 after reset, 8 after an error

    // Values for this sample, worked out before the clock edge. With <= the
    // registers only change after the edge, so a test on "the DLC" at bit 18
    // must use dlc_new, and "is this the last feature bit" must use feat_now.
    wire [3:0]  dlc_new  = {dlc[2:0], lvl};
    wire [3:0]  n_bytes  = rtr ? 4'd0 : (dlc_new > 4'd8 ? 4'd8 : dlc_new);
    wire [3:0]  f_bytes  = (n_bytes > FEAT_BYTES) ? FEAT_BYTES[3:0] : n_bytes;
    wire [6:0]  feat_now = (nbits == 18) ? 7'd19 + 8 * f_bytes : feat_end;
    wire [6:0]  end_now  = (nbits == 18) ? 7'd34 + 8 * n_bytes : frame_end;
    wire [2:0]  run_new  = (lvl == last) ? run + 1'b1 : 3'd1;
    // CRC-15 step, as can_rx.c: shift left, xor 0x4599 if (top bit ^ new bit)
    wire [14:0] crc_new  = {crc[13:0], 1'b0} ^ ((crc[14] ^ lvl) ? 15'h4599 : 15'h0);

    // Still in time for an error flag: anywhere before the end of frame field.
    assign before_eof = (state == IN_FRAME) || (state == TAIL && tail < 3);

    always @(posedge clk) begin
        // One-clock pulses: 0 unless set below.
        frame_valid <= 1'b0;  crc_err  <= 1'b0;  stuff_err <= 1'b0;  form_err <= 1'b0;
        id_ready    <= 1'b0;  dlc_ready <= 1'b0; feat_ready <= 1'b0;

        if (rst) begin
            state     <= WAIT_IDLE;       // bus integration: 11 recessive bits
            rec_run   <= 0;
            idle_need <= 11;
        end else if (sample) begin
            case (state)

            WAIT_IDLE: begin
                if (lvl) begin
                    rec_run <= rec_run + 1'b1;
                    if (rec_run + 1'b1 >= idle_need)
                        state <= IDLE;
                end else begin
                    rec_run <= 0;
                end
            end

            IDLE: begin
                if (!lvl) begin           // start of frame (bit 0)
                    state      <= IN_FRAME;
                    nbits      <= 1;
                    crc        <= 0;      // the dominant SOF leaves the CRC at 0
                    last       <= 1'b0;
                    run        <= 1;
                    stuff_next <= 1'b0;
                    rtr        <= 1'b0;
                    can_id     <= 0;
                    dlc        <= 0;
                    data       <= 0;
                    frame_end  <= 7'h7F;  // unknown until the DLC is in
                    feat_end   <= 7'h7F;
                end
            end

            IN_FRAME: begin
                if (stuff_next) begin
                    // After 5 equal bits the sender adds one opposite bit. It is
                    // not data: no field, no CRC, no nbits.
                    if (lvl == last) begin
                        stuff_err <= 1'b1;
                        state     <= WAIT_IDLE;
                        rec_run   <= 0;
                        idle_need <= 8;
                    end else begin
                        stuff_next <= 1'b0;
                        last       <= lvl;
                        run        <= 1;
                        if (nbits == frame_end) begin   // stuff bit after the last CRC bit
                            state <= TAIL;
                            tail  <= 0;
                        end
                    end
                end else if (nbits == 13 && lvl) begin
                    // IDE = 1: an extended frame. Not in the datasets and not
                    // classified: wait for the bus to go idle, count nothing.
                    state     <= WAIT_IDLE;
                    rec_run   <= 0;
                    idle_need <= 8;
                end else begin
                    // A data bit: CRC, stuffing count, and the field it belongs to.
                    last  <= lvl;
                    run   <= run_new;
                    crc   <= crc_new;
                    nbits <= nbits + 1'b1;

                    if (nbits <= 11) begin                       // ID, bits 1-11
                        can_id <= {can_id[9:0], lvl};
                        if (nbits == 11) id_ready <= 1'b1;
                    end else if (nbits == 12) begin              // RTR
                        rtr <= lvl;
                    end else if (nbits >= 15 && nbits <= 18) begin   // DLC, bits 15-18
                        dlc <= dlc_new;
                        if (nbits == 18) begin
                            frame_end <= end_now;
                            feat_end  <= feat_now;
                            dlc_ready <= 1'b1;
                        end
                    end else if (nbits >= 19 && nbits < frame_end - 15) begin
                        data[63 - (nbits - 19)] <= lvl;          // data bit k -> data[63-k]
                    end

                    if (nbits + 1'b1 == feat_now)                // ID, DLC, first FEAT_BYTES bytes in
                        feat_ready <= 1'b1;
                    if (run_new == 5)
                        stuff_next <= 1'b1;                      // also after the last CRC bit
                    else if (nbits + 1'b1 == end_now) begin      // CRC complete
                        state <= TAIL;
                        tail  <= 0;
                    end
                end
            end

            TAIL: begin
                // tail 0: CRC delimiter, 1: ACK slot, 2: ACK delimiter, 3-9: end of frame
                if (tail == 0 && crc != 0) begin
                    crc_err   <= 1'b1;
                    state     <= WAIT_IDLE;
                    rec_run   <= 0;
                    idle_need <= 8;
                end else if (!lvl && tail != 1) begin
                    if (tail != 9)        // dominant on the last EOF bit: overload, frame stands
                        form_err <= 1'b1;
                    state     <= WAIT_IDLE;
                    rec_run   <= 0;
                    idle_need <= 8;
                end else begin
                    tail <= tail + 1'b1;
                    if (tail == 8) frame_valid <= 1'b1;   // 6th end of frame bit: received
                    if (tail == 9) state <= IDLE;         // 7th: the bus is idle again
                end
            end

            endcase
        end
    end
endmodule
