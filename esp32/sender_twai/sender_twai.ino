/* ESP32 A: CAN frame sender, built in CAN controller (TWAI).
 *
 * For a plain transceiver module with pins TX, RX, VIN, GND, CANH, CANL
 * (SN65HVD230, TJA1050, TJA1051). No extra library needed.
 * The MCP2515 version of this sketch is esp32/sender.
 *
 * The PC (python/esp32_eval.py) streams frames over USB serial and each one
 * goes onto the CAN bus unchanged.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   PC -> board   11 byte packets: id_hi id_lo dlc d0 d1 d2 d3 d4 d5 d6 d7
 *   board -> PC   one byte 'A' after every ACK_EVERY frames queued for the bus
 *
 * Wiring: see docs/ESP32_WIRING.md
 */
#include "driver/twai.h"

#define PIN_CAN_TX  GPIO_NUM_5    // module TX
#define PIN_CAN_RX  GPIO_NUM_4    // module RX
#define GAP_US      0             // extra pause after each frame, if the receiver loses frames
#define ACK_EVERY   64
#define PACKET_LEN  11

void setup()
{
    Serial.setRxBufferSize(4096);
    Serial.begin(921600);

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.tx_queue_len = 64;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();   // the dataset was recorded at 500 kbit/s
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    while (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed. Retrying");
        twai_driver_uninstall();
        delay(500);
    }
    Serial.println("READY");
}

void loop()
{
    static uint8_t pkt[PACKET_LEN];
    static uint32_t sent = 0;

    if (Serial.available() < PACKET_LEN)
        return;
    Serial.readBytes(pkt, PACKET_LEN);

    twai_message_t m = {};
    m.identifier = (((uint32_t)pkt[0] << 8) | pkt[1]) & 0x7FF;
    m.data_length_code = pkt[2] > 8 ? 8 : pkt[2];
    memcpy(m.data, &pkt[3], m.data_length_code);

    // ESP_ERR_TIMEOUT means the TX queue was full and the frame was NOT queued,
    // so retrying cannot duplicate it. With no receiver on the bus nobody ACKs,
    // the queue fills, and this loop waits until one is connected.
    while (twai_transmit(&m, pdMS_TO_TICKS(100)) == ESP_ERR_TIMEOUT) {
    }
    if (GAP_US)
        delayMicroseconds(GAP_US);

    if (++sent % ACK_EVERY == 0)
        Serial.write('A');
}
