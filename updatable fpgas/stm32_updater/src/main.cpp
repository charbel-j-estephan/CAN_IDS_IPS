// STM32 update tool: receives a signed model blob from the PC over USB serial
// and pushes it to the FPGA over SPI. It holds no keys and no model: unplug it
// when the update is done, and the FPGA keeps running from its own flash.
//
// Commands, one per line, 115200 (USB CDC):
//   STATUS      VERSION      COUNTERS      ID
//   TEST <reads> <kHz> <edge 0-3>   SPI link test: counts STATUS reads not starting C1
//   PUSH <len> <crc hex>     then exactly <len> raw bytes follow: upload and COMMIT
#include <Arduino.h>

#include "fpga_update.h"

#define LED PA6

// Printed by ID, so you can tell which build is on the board.
#define FW_ID "updater 3: slow edges, TEST"

static uint8_t blob[24576];

// The longest command is "PUSH 24576 ffffffff" (19 characters). A line longer
// than this is cut: the rest is read and dropped, so a stream of bytes with no
// newline (a PUSH payload sent at the wrong time) cannot grow the String until
// the heap runs out.
#define LINE_MAX 64

static String read_line()
{
    String s;
    s.reserve(LINE_MAX);
    for (;;) {
        while (!Serial.available()) { }
        char c = Serial.read();
        if (c == '\n') return s;
        if (c != '\r' && s.length() < LINE_MAX) s += c;
    }
}

static void print_status()
{
    uint32_t s = fpga_status();
    Serial.printf("STATUS %08lX err=%lu busy=%lu locked=%lu maint=%lu bank=%lu source=%lu\r\n",
                  s, s & 0xFF, (s >> 8) & 1, (s >> 9) & 1, (s >> 10) & 1, (s >> 11) & 1, (s >> 12) & 3);
}

static void print_version()
{
    FpgaVersion v = fpga_version();
    Serial.printf("VERSION model=%08lX version=%lu info=%08lX\r\n", v.model_id, v.version, v.info);
}

static void print_counters()
{
    FpgaCounters c = fpga_counters();
    Serial.printf("COUNTERS seen=%lu blocked=%lu errors=%lu sig_fails=%lu\r\n",
                  c.seen, c.blocked, c.errors, c.sig_fails);
}

static void push(uint32_t len, uint32_t crc)
{
    if (len == 0 || len > sizeof(blob)) {
        Serial.printf("ERROR length %lu (max %u)\r\n", len, (unsigned)sizeof(blob));
        return;
    }
    Serial.printf("READY %lu\r\n", len);
    uint32_t got = 0, t0 = millis();
    while (got < len && millis() - t0 < 10000) {
        if (Serial.available())
            blob[got++] = Serial.read();
    }
    if (got != len) {
        Serial.printf("ERROR received %lu of %lu bytes\r\n", got, len);
        return;
    }
    if (!fpga_upload(blob, len)) {
        Serial.print("ERROR upload refused: ");
        print_status();
        return;
    }
    uint8_t err = fpga_commit(crc);
    if (err == 0xFF)
        Serial.print("ERROR no answer after COMMIT: ");
    else if (err != 0)
        Serial.printf("ERROR COMMIT failed, code %u: ", err);
    else
        Serial.print("OK ");
    print_status();
    print_version();
}

void setup()
{
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    Serial.begin(115200);
    fpga_begin();
}

void loop()
{
    String line = read_line();
    digitalWrite(LED, LOW);
    if (line == "STATUS") print_status();
    else if (line == "VERSION") print_version();
    else if (line == "COUNTERS") print_counters();
    else if (line == "ID") Serial.print("ID " FW_ID "\r\n");
    else if (line.startsWith("TEST ")) {
        // TEST <reads> <kHz> <edge 0-3>: link quality, e.g. TEST 1000 2625 1
        char *end;
        uint32_t n   = strtoul(line.c_str() + 5, &end, 10);
        uint32_t khz = strtoul(end, &end, 10);
        uint32_t spd = strtoul(end, nullptr, 10);
        if (n == 0 || n > 100000 || khz < 100 || khz > 10000 || spd > 3) {
            Serial.print("ERROR TEST <reads 1-100000> <kHz 100-10000> <edge 0-3>\r\n");
        } else {
            uint32_t ff;
            uint32_t bad = fpga_spi_test(n, khz * 1000UL, (uint8_t)spd, &ff);
            Serial.printf("TEST reads=%lu khz=%lu edge=%lu bad=%lu all_ff=%lu\r\n",
                          n, khz, spd, bad, ff);
        }
    }
    else if (line.startsWith("PUSH ")) {
        char *end;
        const char *p = line.c_str() + 5;
        uint32_t len = strtoul(p, &end, 10);
        uint32_t crc = strtoul(end, nullptr, 16);
        push(len, crc);
    } else if (line.length()) {
        Serial.println("ERROR unknown command");
    }
    digitalWrite(LED, HIGH);
}
