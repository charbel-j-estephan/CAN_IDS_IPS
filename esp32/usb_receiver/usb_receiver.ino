/* ESP32 B: CAN receiver that sends every frame to the PC over USB.
 *
 * Same records as wifi_receiver, but over the USB cable, which does not drop
 * data the way a busy WiFi hotspot does. Each frame is stamped with the time it
 * was received (microseconds, this board's clock).
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY\n" once at boot
 *   board -> PC   frame record, 16 bytes: 0xA7 id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                 t = low 32 bits of the microsecond clock, little endian
 *   board -> PC   once a second: "#STAT rx=<frames> ovf=<controller drops> drop=<USB drops> peak=<bytes>\n"
 *
 * At 921600 baud a record takes about 174 us, less than a full 8 byte frame on
 * the bus (about 220 us). A 16 KB send buffer absorbs runs of short frames.
 * "drop" counts frames lost because that buffer was full; it should stay 0.
 *
 * Wiring: module TX -> GPIO 5, module RX -> GPIO 4. See docs/ESP32_WIRING.md
 */
#include "driver/twai.h"
#include "esp_timer.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4
#define TX_BUFFER   16384

static uint32_t rx_count = 0, usb_drop = 0, peak = 0;

void setup()
{
    Serial.setTxBufferSize(TX_BUFFER);
    Serial.begin(921600);

    // Normal mode: this node must ACK frames, or the sender sees no receiver.
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
    static uint32_t last_stat = 0;
    uint32_t now = millis();
    if (now - last_stat >= 1000) {
        last_stat = now;
        twai_status_info_t s;
        uint32_t ovf = 0;
        if (twai_get_status_info(&s) == ESP_OK)
            ovf = s.rx_missed_count + s.rx_overrun_count;
        Serial.printf("#STAT rx=%lu ovf=%lu drop=%lu peak=%lu\n", (unsigned long)rx_count,
                      (unsigned long)ovf, (unsigned long)usb_drop, (unsigned long)peak);
    }

    twai_message_t m;
    if (twai_receive(&m, pdMS_TO_TICKS(5)) != ESP_OK)
        return;
    uint32_t ts = (uint32_t)esp_timer_get_time();   // stamped as soon as the frame is read
    if (m.extd || m.rtr)
        return;

    uint8_t r[16];
    uint8_t len = m.data_length_code > 8 ? 8 : m.data_length_code;
    r[0] = 0xA7;
    r[1] = (m.identifier >> 8) & 0x07;
    r[2] = m.identifier & 0xFF;
    r[3] = len;
    for (int i = 0; i < 8; i++)
        r[4 + i] = i < len ? m.data[i] : 0;
    r[12] = ts;
    r[13] = ts >> 8;
    r[14] = ts >> 16;
    r[15] = ts >> 24;

    int room = Serial.availableForWrite();
    uint32_t used = TX_BUFFER - (room > 0 ? room : 0);
    if (used > peak)
        peak = used;
    if (room < (int)sizeof r) {      // never block: a blocked loop would make the CAN controller drop frames
        usb_drop++;
        return;
    }
    Serial.write(r, sizeof r);
    rx_count++;
}
