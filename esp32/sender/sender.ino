/* ESP32 A: CAN frame sender.
 *
 * The PC (python/esp32_eval.py) reads the dataset and streams frames to this
 * board over USB serial. Each frame is put on the CAN bus unchanged.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   PC -> board   11 byte packets: id_hi id_lo dlc d0 d1 d2 d3 d4 d5 d6 d7
 *   board -> PC   one byte 'A' after every ACK_EVERY frames put on the bus
 *
 * The 'A' bytes are flow control. The PC never has more than two blocks in
 * flight, so the ESP32 serial buffer cannot overflow.
 *
 * Library: "mcp_can" by coryjfowler (Arduino Library Manager).
 * Wiring: see docs/ESP32_WIRING.md
 */
#include <SPI.h>
#include <mcp_can.h>

#define PIN_CS      5
#define CAN_SPEED   CAN_500KBPS   // the Car Hacking Dataset was recorded at 500 kbit/s
#define CAN_CLOCK   MCP_8MHZ      // read the crystal on your board: 8 or 16 MHz
#define GAP_US      0             // extra pause after each frame; raise it if the receiver loses frames
#define ACK_EVERY   64
#define PACKET_LEN  11

MCP_CAN can(PIN_CS);

void setup()
{
    Serial.setRxBufferSize(4096);
    Serial.begin(921600);

    while (can.begin(MCP_ANY, CAN_SPEED, CAN_CLOCK) != CAN_OK) {
        Serial.println("CAN init failed, check wiring and crystal. Retrying");
        delay(500);
    }
    can.setMode(MCP_NORMAL);
    Serial.println("READY");
}

void loop()
{
    static uint8_t pkt[PACKET_LEN];
    static uint32_t sent = 0;

    if (Serial.available() < PACKET_LEN)
        return;
    Serial.readBytes(pkt, PACKET_LEN);

    uint32_t id = ((uint32_t)pkt[0] << 8 | pkt[1]) & 0x7FF;
    uint8_t dlc = pkt[2] > 8 ? 8 : pkt[2];

    // Retry only when no TX buffer was free. CAN_SENDMSGTIMEOUT means the frame
    // IS loaded and the MCP2515 keeps retrying it until someone ACKs, so
    // sending it again would put a duplicate on the bus.
    while (can.sendMsgBuf(id, 0, dlc, &pkt[3]) == CAN_GETTXBFTIMEOUT)
        delayMicroseconds(50);
    if (GAP_US)
        delayMicroseconds(GAP_US);

    if (++sent % ACK_EVERY == 0)
        Serial.write('A');
}
