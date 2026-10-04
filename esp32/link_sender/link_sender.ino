/* Plain CAN sender, no IDS. Sends a frame every 200 ms and prints it.
 *
 * Module pins: TX -> GPIO 5, RX -> GPIO 4, VIN, GND, CANH, CANL to the bus.
 * Open the Serial Monitor at 115200 baud. Pair with link_receiver.
 *
 * Each frame: ID 0x123, 8 data bytes. Bytes 0 to 3 are a counter that goes up
 * by one per frame, bytes 4 to 7 are fixed (DE AD BE EF).
 */
#include "driver/twai.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4

static uint32_t counter = 0;

void setup()
{
    Serial.begin(115200);

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed");
        for (;;)
            delay(1000);
    }
    Serial.println("CAN sender started");
}

void loop()
{
    twai_message_t m = {};
    m.identifier = 0x123;
    m.data_length_code = 8;
    m.data[0] = counter >> 24;
    m.data[1] = counter >> 16;
    m.data[2] = counter >> 8;
    m.data[3] = counter;
    m.data[4] = 0xDE;
    m.data[5] = 0xAD;
    m.data[6] = 0xBE;
    m.data[7] = 0xEF;

    esp_err_t r = twai_transmit(&m, pdMS_TO_TICKS(100));
    if (r == ESP_OK)
        Serial.printf("queued frame %lu\n", (unsigned long)counter);
    else
        Serial.printf("send failed (%d): is the receiver connected?\n", (int)r);

    twai_status_info_t s;
    if (twai_get_status_info(&s) == ESP_OK && s.state != TWAI_STATE_RUNNING)
        Serial.printf("bus state %d: check wiring, CANH/CANL and termination\n", (int)s.state);

    counter++;
    delay(200);
}
