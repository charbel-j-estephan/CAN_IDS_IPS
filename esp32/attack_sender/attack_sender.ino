/* ESP32 C: the attacker. Replays the attack frames of a dataset file at their
 * original times, as its own node on the bus.
 *
 * Same packets and flow control as replay_sender, with one difference: this
 * board does not start its clock when packets arrive over USB (USB timing
 * differs by milliseconds between boards). It waits for the start marker frame
 * that the normal sender (replay_sender) puts on the bus first, and starts its
 * clock from that frame. Both boards then share one time base within a few
 * microseconds, so attack frames land between the normal frames as in the car,
 * and compete with them in real arbitration.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY ATTACKER\n" once at boot
 *   PC -> board   15 byte packets: id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                 t = microseconds after the START of the marker frame
 *   board -> PC   one byte 'A' after every ACK_EVERY frames sent
 *   PC -> board   'Q'  ->  same "#TX ..." line as replay_sender, plus synced=<0|1>
 *
 * Wiring: module TX -> GPIO 5, module RX -> GPIO 4. See docs/ESP32_WIRING.md
 */
#include "driver/twai.h"
#include "esp_timer.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4
#define ACK_EVERY   64
#define PACKET_LEN  15

// Start marker: ID 0x7FF with the bytes "CANSTART". On the wire it is 112 bits
// long, 224 us at 500 kbit/s (python/can_bits.py, stuff bits included). The
// controller reports a frame once it has ended, so the marker STARTED 224 us
// before that, which is the normal sender's time zero.
#define MARKER_ID   0x7FF
#define MARKER_US   224
static const uint8_t MARKER_DATA[8] = {'C', 'A', 'N', 'S', 'T', 'A', 'R', 'T'};

static void report(uint32_t sent, uint32_t late, uint32_t max_late, bool synced)
{
    twai_status_info_t s = {};
    twai_get_status_info(&s);
    const char *state = s.state == TWAI_STATE_RUNNING ? "running"
                      : s.state == TWAI_STATE_BUS_OFF ? "busoff"
                      : s.state == TWAI_STATE_RECOVERING ? "recovering" : "stopped";
    Serial.printf("#TX sent=%lu late=%lu maxlate=%lu tec=%lu rec=%lu state=%s arblost=%lu txfail=%lu buserr=%lu synced=%d\n",
                  (unsigned long)sent, (unsigned long)late, (unsigned long)max_late,
                  (unsigned long)s.tx_error_counter, (unsigned long)s.rx_error_counter, state,
                  (unsigned long)s.arb_lost_count, (unsigned long)s.tx_failed_count,
                  (unsigned long)s.bus_error_count, synced ? 1 : 0);
}

void setup()
{
    Serial.setRxBufferSize(8192);
    Serial.begin(921600);

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.tx_queue_len = 16;
    g.rx_queue_len = 64;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    while (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("CAN init failed. Retrying");
        twai_driver_uninstall();
        delay(500);
    }
    Serial.println("READY ATTACKER");
}

static bool is_marker(const twai_message_t &m)
{
    return !m.extd && !m.rtr && m.identifier == MARKER_ID && m.data_length_code == 8 &&
           memcmp(m.data, MARKER_DATA, 8) == 0;
}

void loop()
{
    static uint8_t pkt[PACKET_LEN];
    static uint32_t sent = 0, late = 0, max_late = 0;
    static int64_t t0 = -1;

    if (Serial.peek() == 'Q' && Serial.available() < PACKET_LEN) {
        Serial.read();
        report(sent, late, max_late, t0 >= 0);
        return;
    }

    // Until the marker is heard, only watch the bus. Packets wait in the USB
    // buffer (the PC never sends more than the buffer holds).
    if (t0 < 0) {
        twai_message_t m;
        while (twai_receive(&m, 0) == ESP_OK) {
            int64_t now = esp_timer_get_time();
            if (is_marker(m)) {
                t0 = now - MARKER_US;
                break;
            }
        }
        return;
    }

    if (Serial.available() < PACKET_LEN)
        return;
    Serial.readBytes(pkt, PACKET_LEN);

    uint32_t rel = (uint32_t)pkt[11] | ((uint32_t)pkt[12] << 8) |
                   ((uint32_t)pkt[13] << 16) | ((uint32_t)pkt[14] << 24);
    int64_t due = t0 + (int64_t)rel;
    while (esp_timer_get_time() < due) {
    }
    int64_t over = esp_timer_get_time() - due;
    if (over > 100) {
        late++;
        if ((uint32_t)over > max_late)
            max_late = (uint32_t)over;
    }

    twai_message_t m = {};
    m.identifier = (((uint32_t)pkt[0] << 8) | pkt[1]) & 0x7FF;
    m.data_length_code = pkt[2] > 8 ? 8 : pkt[2];
    memcpy(m.data, &pkt[3], m.data_length_code);
    while (twai_transmit(&m, pdMS_TO_TICKS(100)) == ESP_ERR_TIMEOUT) {
    }

    if (++sent % ACK_EVERY == 0)
        Serial.write('A');
}
