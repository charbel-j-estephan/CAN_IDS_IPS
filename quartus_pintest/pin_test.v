// Wiring test for the CAN transceiver perfboard on the DE10-Lite. No model,
// no CAN decoding: it only shows what the RX pin sees and lets you drive TX.
// Program it before the real design to check the perfboard.
//
//   GPIO[0] (header pin 1) = transceiver R (RX)   GPIO[1] (header pin 2) = transceiver D (TX)
//
//   LEDR[0]  RX level now: ON = recessive (idle bus, the normal state)
//   LEDR[1]  RX activity: any dominant bit in the last 100 ms
//   LEDR[2]  500 kbit/s seen: the shortest dominant pulse is about 2 us
//   LEDR[3]  echo OK:   TX is dominant (SW[1] up) and RX reads dominant
//   LEDR[4]  echo FAIL: TX is dominant (SW[1] up) but RX still reads recessive
//   LEDR[8]  SW[1] is up: TX is driven dominant
//   LEDR[9]  blinks once a second: the FPGA is programmed and running
//
//   SW[1] up drives TX dominant, holding the whole bus. Flick it up for a
//   second only, with the ESP32s not sending.

module pin_test(
	input        MAX10_CLK1_50,
	input  [9:0] SW,
	output [9:0] LEDR,
	inout  [35:0] GPIO
);
	wire clk = MAX10_CLK1_50;
	localparam [22:0] STRETCH = 23'd5_000_000;   // 100 ms at 50 MHz

	reg [2:0] rx_s = 3'b111;                     // synchronizer + previous value
	reg [1:0] sw_s = 2'b00;
	always @(posedge clk) begin
		rx_s <= {rx_s[1:0], GPIO[0]};
		sw_s <= {sw_s[0], SW[1]};
	end
	wire rx = rx_s[1];
	wire tx_dom = sw_s[1];

	assign GPIO[1]    = tx_dom ? 1'b0 : 1'b1;
	assign GPIO[35:2] = 34'bz;

	// activity
	reg [22:0] act = 0;
	always @(posedge clk)
		act <= !rx ? STRETCH : (act ? act - 1'b1 : 23'd0);

	// shortest dominant pulse in each 0.5 s window; 1 bit at 500 kbit/s = 100 clocks
	reg [15:0] run = 0, min_run = 16'hFFFF, last_min = 16'hFFFF;
	reg [24:0] win = 0;
	always @(posedge clk) begin
		if (!rx)
			run <= (run == 16'hFFFF) ? run : run + 1'b1;
		else begin
			if (run != 0 && run < min_run)
				min_run <= run;
			run <= 0;
		end
		if (win == 25'd24_999_999) begin
			win <= 0;
			last_min <= min_run;
			min_run <= 16'hFFFF;
		end else
			win <= win + 1'b1;
	end
	wire rate_ok = last_min >= 16'd85 && last_min <= 16'd115;

	// heartbeat
	reg [25:0] hb = 0;
	always @(posedge clk)
		hb <= (hb == 26'd49_999_999) ? 26'd0 : hb + 1'b1;

	assign LEDR[0] = rx;
	assign LEDR[1] = act != 0;
	assign LEDR[2] = rate_ok;
	assign LEDR[3] = tx_dom && !rx;
	assign LEDR[4] = tx_dom && rx;
	assign LEDR[7:5] = 3'b0;
	assign LEDR[8] = tx_dom;
	assign LEDR[9] = hb < 26'd25_000_000;
endmodule
