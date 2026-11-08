// STM32 update tool: receives a signed model blob from the PC over USB serial
// and pushes it to the FPGA over SPI. It holds no keys and no model: unplug it
// when the update is done, and the FPGA keeps running from its own flash.
//
// Commands, one per line, 115200 (USB CDC):
//   STATUS      VERSION      COUNTERS
//   PUSH <len> <crc hex>     then exactly <len> raw bytes follow: upload and COMMIT
#include <Arduino.h>

#include "fpga_update.h"

#define LED PA6

static uint8_t blob[24576];

static String read_line()
{
    String s;
    for (;;) {
        while (!Serial.available()) { }
        char c = Serial.read();
        if (c == '\n') return s;
        if (c != '\r') s += c;
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
