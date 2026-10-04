/* Plain CAN receiver, no IDS. Prints every frame it hears.
 *
 * Module pins: TX -> GPIO 5, RX -> GPIO 4, VIN, GND, CANH, CANL to the bus.
 * Open the Serial Monitor at 115200 baud. Pair with link_sender.
 *
 * Works the same for any sender on a 500 kbit/s bus, including the dataset
 * streamer (sender_twai).
 */
#include "driver/twai.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4

static uint32_t count = 0;

void setup()
{
    Serial.begin(115200);

    // Normal mode, not listen only: this node has to ACK frames or the sender
    // will think nobody is there.
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.rx_queue_len = 64;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed");
        for (;;)
            delay(1000);
    }
    Serial.println("CAN receiver started, waiting for frames");
}

void loop()
{
    twai_message_t m;
    if (twai_receive(&m, pdMS_TO_TICKS(1000)) != ESP_OK) {
        Serial.println("no frames in the last second");
        return;
    }

    count++;
    Serial.printf("#%lu  ID 0x%03lX  DLC %d  data:", (unsigned long)count,
                  (unsigned long)m.identifier, m.data_length_code);
    for (int i = 0; i < m.data_length_code; i++)
        Serial.printf(" %02X", m.data[i]);
    Serial.println();
}
