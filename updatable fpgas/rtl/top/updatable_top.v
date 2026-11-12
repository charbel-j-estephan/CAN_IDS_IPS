// Updatable CAN IDS/IPS on the DE10-Lite: board pins, power on reset, the
// On-Chip Flash IP, and updatable_core.
//
//   KEY[0]   reset (hold it down; does not clear the update lockout)
//   SW[0]    prevention on (up) / off (down)
//   SW[1]    maintenance mode (up = the SPI port accepts updates)
//   LEDR[0]  good frame   LEDR[1] attack verdict   LEDR[2] error flag sent
//   LEDR[3]  CRC / stuff / form error
//   LEDR[5:4] where the model came from: 0 none, 1 slot A, 2 slot B, 3 boot ROM
//   LEDR[6]  update port locked out    LEDR[7] update error since reset
//   LEDR[8]  maintenance mode on       LEDR[9] prevention on
//
//   GPIO[0]  CAN RX (header pin 1)     GPIO[1]  CAN TX (header pin 2)
//   GPIO[4]  SPI SCLK (pin 5)          GPIO[5]  SPI MOSI (pin 6)
//   GPIO[6]  SPI MISO (pin 7)          GPIO[7]  SPI CS_n (pin 8)
//
// Program the .pof to keep the design across power cycles, CFM only: programming
// the UFM as well erases the stored models.
//
// The board number and its keys come from keys/device_key.vh, made by
// python/make_device_key.py from your master key (keys/ is not in git). The
// build stops if that file is missing: there is no fallback key. The key ends
// up in the program file and in Quartus' reports: do not share quartus/ outputs.
`include "device_key.vh"

module updatable_top #(
    parameter        READ_MODEL_EN = 1            // 0 for a production build: no READ of the model
) (
    input               ADC_CLK_10,
    input               MAX10_CLK1_50,
    input               MAX10_CLK2_50,
    input        [1:0]  KEY,
    output       [9:0]  LEDR,
    input        [9:0]  SW,
    inout       [35:0]  GPIO
);
    localparam [22:0] STRETCH = 23'd5_000_000;   // LED on time per event: 100 ms

    wire clk = MAX10_CLK1_50;

    // Power on reset: a counter that only a power cycle or a new bitstream restarts.
    reg [4:0] por_cnt = 5'd0;
    wire      por = (por_cnt != 5'd31);
    always @(posedge clk) if (por) por_cnt <= por_cnt + 1'b1;

    reg [1:0] key_s = 2'b11, ips_s = 2'b00, maint_s = 2'b00;
    always @(posedge clk) begin
        key_s   <= {key_s[0], KEY[0]};
        ips_s   <= {ips_s[0], SW[0]};
        maint_s <= {maint_s[0], SW[1]};
    end
    wire rst    = por | ~key_s[1];
    wire ips_on = ips_s[1];

    // Maintenance switch, sampled twice: once every clock and once every 128
    // clocks (2.56 us). Both must say "up", so one glitch on the pin cannot
    // open the update port.
    reg [6:0] maint_t = 7'd0;
    reg       maint_b = 1'b0;
    always @(posedge clk) begin
        maint_t <= maint_t + 1'b1;
        if (maint_t == 7'd0) maint_b <= maint_s[1];
    end
    wire maint  = maint_s[1] & maint_b;

    wire        csr_address, csr_read, csr_write, data_read, data_write, data_waitrequest, data_readdatavalid;
    wire [31:0] csr_writedata, csr_readdata, data_writedata, data_readdata;
    wire [13:0] data_address;
    wire [1:0]  data_burstcount;

    wire        tx_dominant, frame_valid, feat_ready, verdict_valid, verdict_bank, flag_start;
    wire        crc_err, stuff_err, form_err, model_valid, active_bank, spi_miso;
    wire [2:0]  verdict;
    wire [10:0] can_id;
    wire [3:0]  dlc;
    wire [63:0] data;
    wire [31:0] status_word, active_model_id, active_version;

    updatable_core #(.DEVICE_ID(`BOARD_DEVICE_ID), .MAC_KEY(`BOARD_MAC_KEY), .ENC_KEY(`BOARD_ENC_KEY),
                     .READ_MODEL_EN(READ_MODEL_EN)) core (
        .clk(clk), .rst(rst), .por(por), .rxd(GPIO[0]), .tx_dominant(tx_dominant),
        .ips_on(ips_on), .maint(maint),
        .spi_sclk(GPIO[4]), .spi_cs_n(GPIO[7]), .spi_mosi(GPIO[5]), .spi_miso(spi_miso),
        .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount),
        .frame_valid(frame_valid), .feat_ready(feat_ready),
        .verdict_valid(verdict_valid), .verdict(verdict), .verdict_bank(verdict_bank),
        .flag_start(flag_start), .crc_err(crc_err), .stuff_err(stuff_err), .form_err(form_err),
        .can_id(can_id), .dlc(dlc), .data(data), .status_word(status_word),
        .active_model_id(active_model_id), .active_version(active_version),
        .model_valid(model_valid), .active_bank(active_bank)
    );

    nv_flash flash (
        .clk_clk(clk), .nreset_reset_n(~rst),
        .csr_address(csr_address), .csr_read(csr_read), .csr_writedata(csr_writedata),
        .csr_write(csr_write), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_writedata(data_writedata),
        .data_write(data_write), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount)
    );

    // TX: 1 = recessive. Driven high from power up.
    assign GPIO[1]    = ~tx_dominant;
    assign GPIO[6]    = GPIO[7] ? 1'bz : spi_miso;   // MISO only while selected
    assign GPIO[3:2]  = 2'bzz;
    assign GPIO[35:8] = 28'bz;

    reg [22:0] led_frame = 0, led_attack = 0, led_flag = 0, led_err = 0;
    reg        led_upd   = 0;
    always @(posedge clk) begin
        led_frame  <= frame_valid                     ? STRETCH : (led_frame  ? led_frame  - 1'b1 : 23'd0);
        led_attack <= (verdict_valid && verdict != 0) ? STRETCH : (led_attack ? led_attack - 1'b1 : 23'd0);
        led_flag   <= flag_start                      ? STRETCH : (led_flag   ? led_flag   - 1'b1 : 23'd0);
        led_err    <= (crc_err || stuff_err || form_err) ? STRETCH : (led_err ? led_err - 1'b1 : 23'd0);
        led_upd    <= rst ? 1'b0 : (led_upd | (status_word[7:0] != 8'd0));
    end

    assign LEDR[0]   = led_frame  != 0;
    assign LEDR[1]   = led_attack != 0;
    assign LEDR[2]   = led_flag   != 0;
    assign LEDR[3]   = led_err    != 0;
    assign LEDR[5:4] = status_word[13:12];
    assign LEDR[6]   = status_word[9];
    assign LEDR[7]   = led_upd;
    assign LEDR[8]   = maint;
    assign LEDR[9]   = ips_on;
endmodule
