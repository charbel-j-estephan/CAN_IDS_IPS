/* ESP32 B: pass-through receiver for the STM32 gateway setup.
 *
 * Sits on CAN bus 2, behind the STM32, and reports every frame that arrives to
 * the PC. No model here: the STM32 decides what gets through, and the PC works
 * out accuracy from which frames show up (python/esp32_eval.py --gateway).
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   board -> PC   per frame, 12 bytes: 0xA6 id_hi id_lo dlc d0 d1 d2 d3 d4 d5 d6 d7
 *   PC -> board   'Q'  ->  "#DONE rx=<frames> ovf=<frames the controller dropped>\n"
 *   PC -> board   'Z'  ->  zero the counters
 *
 * At 921600 baud a 12 byte record takes about 130 us, slower than a full
 * 8 byte frame on the bus (about 110 us). That is why the sender pauses
 * GAP_US = 150 us between frames. Do not lower it with this sketch.
 *
 * Wiring: see docs/ESP32_WIRING.md
 */
#include "driver/twai.h"

#define PIN_CAN_TX  GPIO_NUM_5    // module TX
#define PIN_CAN_RX  GPIO_NUM_4    // module RX

static uint32_t rx_count = 0;

void setup()
{
    Serial.begin(921600);

    // Normal mode, not listen only: this node must ACK, or the STM32's CAN2
    // would see no acknowledgement and retry forever.
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.rx_queue_len = 256;
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
    while (Serial.available()) {
        int c = Serial.read();
        if (c == 'Q') {
            twai_status_info_t s;
            uint32_t dropped = 0;
            if (twai_get_status_info(&s) == ESP_OK)
                dropped = s.rx_missed_count + s.rx_overrun_count;
            Serial.printf("#DONE rx=%lu ovf=%lu\n", (unsigned long)rx_count, (unsigned long)dropped);
        } else if (c == 'Z') {
            rx_count = 0;
        }
    }

    twai_message_t m;
    if (twai_receive(&m, pdMS_TO_TICKS(5)) != ESP_OK)
        return;
    if (m.extd || m.rtr)
        return;

    uint8_t len = m.data_length_code > 8 ? 8 : m.data_length_code;
    uint8_t out[12] = {0xA6, (uint8_t)((m.identifier >> 8) & 0x07), (uint8_t)(m.identifier & 0xFF), len};
    for (int i = 0; i < len; i++)
        out[4 + i] = m.data[i];
    Serial.write(out, sizeof out);
    rx_count++;
}
