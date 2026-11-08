// SPI master for the testbenches: what the STM32 update tool does. Included
// inside a module that has clk, sclk, cs_n, mosi, miso and a reg [7:0] blob[].
`ifndef SPI_HALF_NS
`define SPI_HALF_NS 190
`endif

    reg  [7:0] spi_r;

    task spi_byte;
        input [7:0] tx;
        integer b;
        begin
            for (b = 7; b >= 0; b = b - 1) begin
                mosi = tx[b];
                #(`SPI_HALF_NS);
                sclk = 1'b1;
                spi_r[b] = miso;
                #(`SPI_HALF_NS);
                sclk = 1'b0;
            end
        end
    endtask

    task cs_low;  begin cs_n = 1'b0; #(`SPI_HALF_NS); end endtask
    task cs_high; begin #(`SPI_HALF_NS); cs_n = 1'b1; #(`SPI_HALF_NS * 2); end endtask

    // Upload `n` bytes of blob[] in chunks, in order.
    task spi_upload;
        input integer n;
        integer off, chunk, c;
        begin
            off = 0;
            while (off < n) begin
                chunk = (n - off > 256) ? 256 : (n - off);
                cs_low;
                spi_byte(8'h01);
                spi_byte(off[15:8]);  spi_byte(off[7:0]);
                spi_byte(chunk[15:8]); spi_byte(chunk[7:0]);
                for (c = 0; c < chunk; c = c + 1) spi_byte(blob[off + c]);
                cs_high;
                off = off + chunk;
            end
        end
    endtask

    reg [31:0] spi_word;
    task spi_read_word;
        integer i;
        begin
            spi_word = 32'd0;
            for (i = 0; i < 4; i = i + 1) begin
                spi_byte(8'hFF);
                spi_word = {spi_word[23:0], spi_r};
            end
        end
    endtask

    reg [31:0] status_now;
    task spi_status;
        begin
            cs_low;
            spi_byte(8'h04);
            spi_byte(8'hFF);
            spi_read_word;
            status_now = spi_word;
            cs_high;
        end
    endtask

    reg [31:0] ver_model, ver_version, ver_info;
    task spi_version;
        begin
            cs_low;
            spi_byte(8'h05);
            spi_byte(8'hFF);
            spi_read_word; ver_model   = spi_word;
            spi_read_word; ver_version = spi_word;
            spi_read_word; ver_info    = spi_word;
            cs_high;
        end
    endtask

    reg [31:0] cnt_seen, cnt_blocked, cnt_errors, cnt_sig;
    task spi_counters;
        begin
            cs_low;
            spi_byte(8'h06);
            spi_byte(8'hFF);
            spi_read_word; cnt_seen    = spi_word;
            spi_read_word; cnt_blocked = spi_word;
            spi_read_word; cnt_errors  = spi_word;
            spi_read_word; cnt_sig     = spi_word;
            cs_high;
        end
    endtask

    reg [31:0] reg_word;
    task spi_read_reg;
        input [15:0] a;
        begin
            cs_low;
            spi_byte(8'h02);
            spi_byte(a[15:8]);
            spi_byte(a[7:0]);
            spi_byte(8'hFF);
            spi_read_word;
            reg_word = spi_word;
            cs_high;
        end
    endtask

    // COMMIT and wait for the FPGA to finish; status_now holds the final status.
    task spi_commit;
        input [31:0] crc;
        begin
            cs_low;
            spi_byte(8'h03);
            spi_byte(crc[31:24]); spi_byte(crc[23:16]);
            spi_byte(crc[15:8]);  spi_byte(crc[7:0]);
            cs_high;
            spi_status;
            while (status_now[8] || status_now[15]) begin     // busy or swap pending
                #20000;
                spi_status;
            end
        end
    endtask

    task wait_boot;
        begin
            spi_status;
            while (status_now[8] || status_now[15] || status_now[13:12] == 2'd0) spi_status;
        end
    endtask
