// SPI driver for the FPGA's update port: SPI2, mode 0, 2.6 MHz, 3.3 V, wired
// directly to the DE10-Lite header.
//   PB13 SCLK -> pin 5   PB15 MOSI -> pin 6   PB14 MISO <- pin 7   PB12 CS_n -> pin 8
#include "fpga_update.h"

#include <SPI.h>

#define FPGA_CS   PB12

static SPIClass fpga_spi(PB15, PB14, PB13);
static const SPISettings FPGA_SPI(2625000, MSBFIRST, SPI_MODE0);

enum { C_WRITE = 0x01, C_READ = 0x02, C_COMMIT = 0x03, C_STATUS = 0x04, C_VERSION = 0x05, C_COUNTERS = 0x06 };

static inline void select()   { digitalWrite(FPGA_CS, LOW);  delayMicroseconds(2); }
static inline void deselect() { delayMicroseconds(2); digitalWrite(FPGA_CS, HIGH); delayMicroseconds(5); }

static uint32_t read_word()
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
        v = (v << 8) | fpga_spi.transfer(0xFF);
    return v;
}

void fpga_begin()
{
    pinMode(FPGA_CS, OUTPUT);
    digitalWrite(FPGA_CS, HIGH);
    fpga_spi.begin();
    fpga_spi.beginTransaction(FPGA_SPI);
}

uint32_t fpga_status()
{
    select();
    fpga_spi.transfer(C_STATUS);
    fpga_spi.transfer(0xFF);
    uint32_t v = read_word();
    deselect();
    return v;
}

FpgaVersion fpga_version()
{
    FpgaVersion v;
    select();
    fpga_spi.transfer(C_VERSION);
    fpga_spi.transfer(0xFF);
    v.model_id = read_word();
    v.version  = read_word();
    v.info     = read_word();
    deselect();
    return v;
}

FpgaCounters fpga_counters()
{
    FpgaCounters c;
    select();
    fpga_spi.transfer(C_COUNTERS);
    fpga_spi.transfer(0xFF);
    c.seen      = read_word();
    c.blocked   = read_word();
    c.errors    = read_word();
    c.sig_fails = read_word();
    deselect();
    return c;
}

uint32_t fpga_read(uint16_t addr)
{
    select();
    fpga_spi.transfer(C_READ);
    fpga_spi.transfer(addr >> 8);
    fpga_spi.transfer(addr & 0xFF);
    fpga_spi.transfer(0xFF);
    uint32_t v = read_word();
    deselect();
    return v;
}

bool fpga_upload(const uint8_t *blob, uint32_t len)
{
    uint32_t off = 0;
    while (off < len) {
        uint16_t chunk = (len - off > 256) ? 256 : (uint16_t)(len - off);
        select();
        fpga_spi.transfer(C_WRITE);
        fpga_spi.transfer(off >> 8);
        fpga_spi.transfer(off & 0xFF);
        fpga_spi.transfer(chunk >> 8);
        fpga_spi.transfer(chunk & 0xFF);
        for (uint16_t i = 0; i < chunk; i++)
            fpga_spi.transfer(blob[off + i]);
        deselect();
        off += chunk;
        // A refused chunk shows as a new error code; keep going would waste time.
        if (((fpga_status() >> 0) & 0xFF) != 0)
            return false;
    }
    return true;
}

uint8_t fpga_commit(uint32_t crc, uint32_t timeout_ms)
{
    select();
    fpga_spi.transfer(C_COMMIT);
    fpga_spi.transfer((crc >> 24) & 0xFF);
    fpga_spi.transfer((crc >> 16) & 0xFF);
    fpga_spi.transfer((crc >> 8) & 0xFF);
    fpga_spi.transfer(crc & 0xFF);
    deselect();

    uint32_t t0 = millis();
    while (millis() - t0 < timeout_ms) {
        uint32_t s = fpga_status();
        if ((s >> 24) == 0xC1 && !(s & (1UL << 8)) && !(s & (1UL << 15)))
            return (uint8_t)(s & 0xFF);
        delay(5);
    }
    return 0xFF;
}
