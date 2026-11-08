// SPI driver for the FPGA's update port. See docs/REGISTER_MAP.md.
#pragma once
#include <Arduino.h>

void fpga_begin();

struct FpgaVersion {
    uint32_t model_id;
    uint32_t version;
    uint32_t info;
};

struct FpgaCounters {
    uint32_t seen;
    uint32_t blocked;
    uint32_t errors;
    uint32_t sig_fails;
};

uint32_t fpga_status();
FpgaVersion fpga_version();
FpgaCounters fpga_counters();
uint32_t fpga_read(uint16_t addr);

// Sends the blob in 256 byte chunks. Returns false if the FPGA refused a chunk.
bool fpga_upload(const uint8_t *blob, uint32_t len);

// Sends COMMIT and polls until the FPGA is done, at most timeout_ms.
// Returns LAST_ERROR: 0 = the new model is active. 0xFF = no answer in time.
uint8_t fpga_commit(uint32_t crc, uint32_t timeout_ms = 5000);
