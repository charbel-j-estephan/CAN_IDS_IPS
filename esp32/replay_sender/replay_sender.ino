/* ESP32 A: replays dataset frames onto the CAN bus with their original timing.
 *
 * The PC (python/replay_dataset.py) streams frames over USB serial, each with
 * the time it had in the original capture. This board waits until that moment
 * and then sends the frame, so the bus carries the same traffic pattern the
 * car did. Use a data cable: the PC needs this board's COM port.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   PC -> board   15 byte packets: id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                 t is a 32 bit little endian count of microseconds since the
 *                 first frame. t = 0 means "send now" (used for the fast mode).
 *   board -> PC   one byte 'A' after every ACK_EVERY frames sent
 *   PC -> board   'Q'  ->  "#TX sent=<n> late=<n> maxlate=<us>\n"
 *
 * "late" counts frames sent more than 100 us after their time (the bus was
 * busy, or the PC did not deliver in time).
 *
 * Wiring: module TX -> GPIO 5, module RX -> GPIO 4. See docs/ESP32_WIRING.md
 */
#include "driver/twai.h"
#include "esp_timer.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4
#define ACK_EVERY   64
#define PACKET_LEN  15

void setup()
{
    Serial.setRxBufferSize(8192);
    Serial.begin(921600);

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.tx_queue_len = 16;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
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
    static uint32_t sent = 0, late = 0, max_late = 0;
    static int64_t t0 = -1;

    if (Serial.peek() == 'Q' && Serial.available() < PACKET_LEN) {
        Serial.read();
        Serial.printf("#TX sent=%lu late=%lu maxlate=%lu\n",
                      (unsigned long)sent, (unsigned long)late, (unsigned long)max_late);
        return;
    }
    if (Serial.available() < PACKET_LEN)
        return;
    Serial.readBytes(pkt, PACKET_LEN);

    uint32_t rel = (uint32_t)pkt[11] | ((uint32_t)pkt[12] << 8) |
                   ((uint32_t)pkt[13] << 16) | ((uint32_t)pkt[14] << 24);
    if (t0 < 0)
        t0 = esp_timer_get_time();       // the first frame defines time zero

    if (rel) {
        int64_t due = t0 + (int64_t)rel;
        while (esp_timer_get_time() < due) {
        }
        int64_t over = esp_timer_get_time() - due;
        if (over > 100) {
            late++;
            if ((uint32_t)over > max_late)
                max_late = (uint32_t)over;
        }
    }

    twai_message_t m = {};
    m.identifier = (((uint32_t)pkt[0] << 8) | pkt[1]) & 0x7FF;
    m.data_length_code = pkt[2] > 8 ? 8 : pkt[2];
    memcpy(m.data, &pkt[3], m.data_length_code);

    // A timeout means the TX queue was full, the frame was not queued, so
    // retrying cannot duplicate it.
    while (twai_transmit(&m, pdMS_TO_TICKS(100)) == ESP_ERR_TIMEOUT) {
    }

    if (++sent % ACK_EVERY == 0)
        Serial.write('A');
}
