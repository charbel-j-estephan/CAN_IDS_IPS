/* ESP32 B: CAN receiver and intrusion detector.
 *
 * Every frame on the bus runs through the same random forest that the FPGA and
 * STM32 builds use (emlearn C code from python/export_c.py). The result goes
 * to the PC over USB serial, where python/esp32_eval.py scores it.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   board -> PC   per frame, 4 bytes: 0xA5 class id_hi id_lo
 *   PC -> board   'Q'  ->  "#DONE rx=<frames> ovf=<0 or 1>\n"
 *   PC -> board   'Z'  ->  zero the counters
 *
 * Needs can_ids_model.h in this folder. esp32_eval.py --export copies it from
 * mcu/generated/ (run python/export_c.py first).
 *
 * Library: "mcp_can" by coryjfowler (Arduino Library Manager).
 * Wiring: see docs/ESP32_WIRING.md
 */
#include <SPI.h>
#include <mcp_can.h>

#include "can_ids_model.h"

#define PIN_CS      5
#define PIN_INT     4
#define CAN_SPEED   CAN_500KBPS   // must match the sender
#define CAN_CLOCK   MCP_8MHZ      // must match the sender's module crystal

MCP_CAN can(PIN_CS);

static uint32_t rx_count = 0;
static uint8_t ovf_seen = 0;   // sticky: the MCP2515 flag stays set once an RX buffer overflowed

void setup()
{
    Serial.begin(921600);
    pinMode(PIN_INT, INPUT);

    while (can.begin(MCP_ANY, CAN_SPEED, CAN_CLOCK) != CAN_OK) {
        Serial.println("CAN init failed, check wiring and crystal. Retrying");
        delay(500);
    }
    can.setMode(MCP_NORMAL);   // normal mode, so this node ACKs frames
    Serial.println("READY");
}

void loop()
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == 'Q') {
            Serial.printf("#DONE rx=%lu ovf=%u\n",
                          (unsigned long)rx_count, (unsigned)ovf_seen);
        } else if (c == 'Z') {
            rx_count = 0;
        }
    }

    if (digitalRead(PIN_INT) != LOW)   // INT is active low while a frame waits
        return;

    unsigned long id;
    byte len;
    byte buf[8] = {0};                 // bytes past the DLC must read as 0, as in training
    if (can.readMsgBuf(&id, &len, buf) != CAN_OK)
        return;
    if (id & 0xC0000000UL)             // mcp_can puts the extended and remote flags in the ID
        return;

    float feat[10];
    feat[0] = (float)(id & 0x7FF);
    feat[1] = (float)(len > 8 ? 8 : len);
    for (int i = 0; i < 8; i++)
        feat[2 + i] = (i < len) ? (float)buf[i] : 0.0f;

    int32_t cls = can_ids_predict(feat, 10);
    rx_count++;

    uint8_t out[4] = {0xA5, (uint8_t)cls, (uint8_t)((id >> 8) & 0x07), (uint8_t)(id & 0xFF)};
    Serial.write(out, sizeof out);

    // Error flags cost SPI reads, so look only every 256 frames. The overflow
    // flag is sticky, so it is still caught.
    if ((rx_count & 0xFF) == 0 && can.checkError() == CAN_CTRLERROR) {
        if (can.getError() & 0xC0)             // RX1OVR | RX0OVR
            ovf_seen = 1;
    }
}
