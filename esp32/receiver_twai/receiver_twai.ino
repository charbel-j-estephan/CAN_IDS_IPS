/* ESP32 B: CAN receiver and intrusion detector, built in CAN controller (TWAI).
 *
 * For a plain transceiver module with pins TX, RX, VIN, GND, CANH, CANL.
 * No extra library needed. The MCP2515 version is esp32/receiver.
 *
 * Every frame on the bus runs through the same random forest the FPGA and
 * STM32 builds use (emlearn C code from python/export_c.py). The result goes
 * to the PC over USB serial, where python/esp32_eval.py scores it.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   board -> PC   per frame, 4 bytes: 0xA5 class id_hi id_lo
 *   PC -> board   'Q'  ->  "#DONE rx=<frames> ovf=<frames the controller dropped>\n"
 *   PC -> board   'Z'  ->  zero the counters
 *
 * Needs can_ids_model.h in this folder: run python/esp32_eval.py --copy-model
 * after python/export_c.py.
 *
 * Wiring: see docs/ESP32_WIRING.md
 */
#include "driver/twai.h"

#include "can_ids_model.h"

#define PIN_CAN_TX  GPIO_NUM_5    // module TX
#define PIN_CAN_RX  GPIO_NUM_4    // module RX

static uint32_t rx_count = 0;
static uint32_t dropped = 0;      // frames the CAN controller lost because we read too slowly

void setup()
{
    Serial.begin(921600);

    // TWAI_MODE_NORMAL, not listen only: this node must ACK frames, otherwise
    // the sender sees no acknowledgement and retries forever.
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.rx_queue_len = 256;         // absorbs bursts while the forest runs
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();   // must match the sender
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    while (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed. Retrying");
        twai_driver_uninstall();
        delay(500);
    }
    Serial.println("READY");
}

static void update_dropped()
{
    twai_status_info_t s;
    if (twai_get_status_info(&s) == ESP_OK)
        dropped = s.rx_missed_count + s.rx_overrun_count;
}

void loop()
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == 'Q') {
            update_dropped();
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

    float feat[10];
    uint8_t len = m.data_length_code > 8 ? 8 : m.data_length_code;
    feat[0] = (float)(m.identifier & 0x7FF);
    feat[1] = (float)len;
    for (int i = 0; i < 8; i++)
        feat[2 + i] = (i < len) ? (float)m.data[i] : 0.0f;   // bytes past the DLC are 0, as in training

    int32_t cls = can_ids_predict(feat, 10);
    rx_count++;

    uint8_t out[4] = {0xA5, (uint8_t)cls, (uint8_t)((m.identifier >> 8) & 0x07), (uint8_t)(m.identifier & 0xFF)};
    Serial.write(out, sizeof out);
}
