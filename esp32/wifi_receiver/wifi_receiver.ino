/* ESP32 B: CAN receiver that streams every frame to the PC over WiFi.
 *
 * No USB data connection needed: power it from any USB cable. Each frame is
 * stamped with the time it was received (microseconds, this board's clock) and
 * sent to python/replay_dataset.py, which stores all of them.
 *
 * Setup: copy secrets.example.h to secrets.h in this folder and fill in your
 * hotspot name, password and the PC's IP address on that hotspot.
 *
 * TCP stream to the PC (this board connects to the PC):
 *   frame record, 16 bytes: 0xA7 id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                           t = low 32 bits of the microsecond clock, little endian
 *   text line, once a second: "#STAT rx=<frames> ovf=<controller drops> drop=<ring drops>\n"
 *
 * Frames pass through a ring buffer so a slow WiFi moment does not make the
 * CAN controller lose frames. "drop" counts frames lost if that ring overflows.
 *
 * Wiring: module TX -> GPIO 5, module RX -> GPIO 4. See docs/ESP32_WIRING.md
 */
#include <WiFi.h>

#include "driver/twai.h"
#include "esp_timer.h"

#include "secrets.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4
#define RING_SIZE   6144          // records, 16 bytes each = 96 KB
#define BATCH       64            // records per TCP write

static uint8_t (*ring)[16];
static volatile uint32_t head = 0;       // written by the CAN loop
static volatile uint32_t tail = 0;       // written by the network task
static volatile uint32_t rx_count = 0;
static volatile uint32_t ring_drop = 0;

static void net_task(void *)
{
    WiFiClient client;
    uint32_t last_stat = 0;
    static uint8_t out[BATCH * 16 + 96];

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if (!client.connected()) {
            client.stop();
            if (!client.connect(PC_IP, PC_PORT)) {
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            client.setNoDelay(true);
        }

        size_t n = 0;
        uint32_t t = tail;
        while (t != head && n < BATCH * 16) {
            memcpy(out + n, ring[t % RING_SIZE], 16);
            n += 16;
            t++;
        }

        uint32_t now = millis();
        if (now - last_stat >= 1000) {
            last_stat = now;
            twai_status_info_t s;
            uint32_t ovf = 0;
            if (twai_get_status_info(&s) == ESP_OK)
                ovf = s.rx_missed_count + s.rx_overrun_count;
            n += snprintf((char *)out + n, 80, "#STAT rx=%lu ovf=%lu drop=%lu\n",
                          (unsigned long)rx_count, (unsigned long)ovf, (unsigned long)ring_drop);
        }

        if (n) {
            if (client.write(out, n) == n) {
                tail = t;
            } else {
                client.stop();      // the PC closed the connection, reconnect
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

void setup()
{
    Serial.begin(115200);
    ring = (uint8_t(*)[16])malloc((size_t)RING_SIZE * 16);

    // Normal mode: this node must ACK frames, or the sender sees no receiver.
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.rx_queue_len = 128;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    while (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed. Retrying");
        twai_driver_uninstall();
        delay(500);
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);               // power saving adds delay and jitter
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi connected, this board is %s, streaming to %s:%d\n",
                  WiFi.localIP().toString().c_str(), PC_IP, PC_PORT);

    xTaskCreatePinnedToCore(net_task, "net", 8192, NULL, 1, NULL, 0);
}

void loop()
{
    twai_message_t m;
    if (twai_receive(&m, pdMS_TO_TICKS(5)) != ESP_OK)
        return;
    uint32_t ts = (uint32_t)esp_timer_get_time();   // stamped as soon as the frame is read
    if (m.extd || m.rtr)
        return;

    uint32_t h = head;
    if (h - tail >= RING_SIZE) {
        ring_drop++;
        return;
    }
    uint8_t *r = ring[h % RING_SIZE];
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
    head = h + 1;
    rx_count++;
}
