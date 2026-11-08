// SPI slave, mode 0, MSB first, all in the system clock domain: SCLK, CS_n and
// MOSI are synchronized and SCLK's edges detected, so there is one clock domain
// (also easier for the ASIC). SCLK must stay well below clk/8; the update tool
// uses 2.6 MHz.
//
//   rx_valid   1 clock after the 8th rising edge of a byte, with rx_byte
//   tx_byte    the next byte to send: read when CS_n falls and at the falling
//              edge that follows each byte's last bit
//   frame_end  1 clock when CS_n rises
module spi_slave (
    input  wire       clk,
    input  wire       rst,
    input  wire       sclk,
    input  wire       cs_n,
    input  wire       mosi,
    output reg        miso = 1'b1,
    output reg        rx_valid = 1'b0,
    output reg  [7:0] rx_byte = 8'h00,
    input  wire [7:0] tx_byte,
    output reg        frame_start = 1'b0,
    output reg        frame_end = 1'b0
);
    reg [2:0] sclk_s = 3'b000;
    reg [2:0] cs_s   = 3'b111;
    reg [1:0] mosi_s = 2'b00;
    always @(posedge clk) begin
        sclk_s <= {sclk_s[1:0], sclk};
        cs_s   <= {cs_s[1:0], cs_n};
        mosi_s <= {mosi_s[0], mosi};
    end
    wire rise    = (sclk_s[2:1] == 2'b01);
    wire fall    = (sclk_s[2:1] == 2'b10);
    wire selected = !cs_s[1];
    wire cs_fall = (cs_s[2:1] == 2'b10);
    wire cs_rise = (cs_s[2:1] == 2'b01);

    reg [2:0] bit_i   = 3'd0;
    reg [6:0] rx_sh   = 7'd0;
    reg [7:0] tx_sh   = 8'hFF;
    reg       reload  = 1'b0;      // last rising edge ended a byte

    always @(posedge clk) begin
        rx_valid    <= 1'b0;
        frame_start <= 1'b0;
        frame_end   <= 1'b0;
        if (rst) begin
            bit_i  <= 3'd0;
            reload <= 1'b0;
            miso   <= 1'b1;
        end else begin
            if (cs_fall) begin
                bit_i       <= 3'd0;
                reload      <= 1'b0;
                frame_start <= 1'b1;
                tx_sh       <= tx_byte;
                miso        <= tx_byte[7];
            end else if (cs_rise) begin
                frame_end <= 1'b1;
                miso      <= 1'b1;
            end else if (selected) begin
                if (rise) begin
                    rx_sh <= {rx_sh[5:0], mosi_s[1]};
                    bit_i <= bit_i + 1'b1;
                    if (bit_i == 3'd7) begin
                        rx_valid <= 1'b1;
                        rx_byte  <= {rx_sh, mosi_s[1]};
                        reload   <= 1'b1;
                    end
                end
                if (fall) begin
                    if (reload) begin
                        reload <= 1'b0;
                        tx_sh  <= tx_byte;
                        miso   <= tx_byte[7];
                    end else begin
                        tx_sh  <= {tx_sh[6:0], 1'b1};
                        miso   <= tx_sh[6];
                    end
                end
            end
        end
    end
endmodule
