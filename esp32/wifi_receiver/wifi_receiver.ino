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
#define CHUNK       1024          // records per memory block (16 KB)
#define MAX_CHUNKS  6             // 6144 records, 96 KB, about 4 s of a busy bus
#define KEEP_FREE   100000        // bytes of memory left for WiFi; less made sends fail
#define BATCH       64            // records per TCP write (1 KB); 4 KB writes starved WiFi

// The buffer is built from 16 KB blocks: the ESP32 rarely has one free block
// big enough for the whole thing, but it has many small ones.
static uint8_t (*chunks[MAX_CHUNKS])[16];
static uint32_t ring_size = 0;
static volatile uint32_t peak = 0;       // most records ever waiting for WiFi

static inline uint8_t *slot(uint32_t i)
{
    i %= ring_size;
    return chunks[i / CHUNK][i % CHUNK];
}
static volatile uint32_t head = 0;       // written by the CAN loop
static volatile uint32_t tail = 0;       // written by the network task
static volatile uint32_t rx_count = 0;
static volatile uint32_t ring_drop = 0;

// WiFiClient.write() may accept only part of the data when the TCP buffer is
// full. Keep writing until everything is out, and give up only if the
// connection is really gone.
static bool write_all(WiFiClient &c, const uint8_t *p, size_t n)
{
    size_t off = 0;
    uint32_t last_progress = millis();
    while (off < n) {
        size_t w = c.write(p + off, n - off);
        if (w > 0) {
            off += w;
            last_progress = millis();
            continue;
        }
        if (!c.connected() || millis() - last_progress > 5000)
            return false;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return true;
}

static void net_task(void *)
{
    WiFiClient client;
    uint32_t last_stat = 0;
    static uint8_t out[BATCH * 16];
    char line[192];

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
            memcpy(out + n, slot(t), 16);
            n += 16;
            t++;
        }

        if (n) {
            if (!write_all(client, out, n)) {
                client.stop();      // connection lost: these records are resent after reconnecting
                continue;
            }
            tail = t;
        }

        uint32_t now = millis();
        if (now - last_stat >= 1000) {
            last_stat = now;
            twai_status_info_t s;
            uint32_t ovf = 0;
            if (twai_get_status_info(&s) == ESP_OK)
                ovf = s.rx_missed_count + s.rx_overrun_count;
            int len = snprintf(line, sizeof line,
                               "#STAT rx=%lu ovf=%lu drop=%lu peak=%lu size=%lu heap=%lu minheap=%lu\n",
                               (unsigned long)rx_count, (unsigned long)ovf, (unsigned long)ring_drop,
                               (unsigned long)peak, (unsigned long)ring_size,
                               (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap());
            if (len < 0 || len >= (int)sizeof line)
                len = strlen(line);
            if (!write_all(client, (const uint8_t *)line, len)) {
                client.stop();
                continue;
            }
        }

        if (!n)
            vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void setup()
{
    Serial.begin(115200);

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

    // Say why a connection fails instead of printing dots forever.
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        uint8_t r = info.wifi_sta_disconnected.reason;
        const char *why = r == 201 ? "network not found: wrong name, or it is 5 GHz only"
                        : (r == 15 || r == 202 || r == 204 || r == 2) ? "wrong password, or WPA3-only security"
                        : "see the reason code";
        Serial.printf("WiFi failed, reason %u: %s\n", r, why);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    Serial.println("Networks this board can see (it only sees 2.4 GHz):");
    int found = WiFi.scanNetworks();
    for (int i = 0; i < found; i++)
        Serial.printf("  %-32s signal %d dBm  channel %d%s\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                      WiFi.channel(i), WiFi.SSID(i) == WIFI_SSID ? "   <- yours" : "");
    if (found <= 0)
        Serial.println("  none");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to \"%s\"", WIFI_SSID);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() - start > 20000) {
            Serial.println("\nStill not connected after 20 s, retrying");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            start = millis();
        }
    }
    Serial.printf("\nWiFi connected, this board is %s, streaming to %s:%d\n",
                  WiFi.localIP().toString().c_str(), PC_IP, PC_PORT);

    // Reserve the frame buffer only now, after WiFi has taken the memory it needs.
    for (int c = 0; c < MAX_CHUNKS && ESP.getFreeHeap() > KEEP_FREE + CHUNK * 16; c++) {
        chunks[c] = (uint8_t(*)[16])malloc(CHUNK * 16);
        if (!chunks[c])
            break;
        ring_size += CHUNK;
    }
    Serial.printf("Buffer holds %lu frames (%lu bytes of memory left)\n",
                  (unsigned long)ring_size, (unsigned long)ESP.getFreeHeap());

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
    uint32_t waiting = h - tail;
    if (waiting >= ring_size) {
        ring_drop++;
        return;
    }
    if (waiting + 1 > peak)
        peak = waiting + 1;
    uint8_t *r = slot(h);
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
