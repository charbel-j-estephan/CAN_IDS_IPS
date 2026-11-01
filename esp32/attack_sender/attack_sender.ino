/* Attacker: replays the attack frames of a dataset file at their
 * original times, as its own node on the bus.
 *
 * Same packets and flow control as replay_sender, with two differences.
 *
 * 1. This board does not start its clock when packets arrive over USB (USB
 *    timing differs by milliseconds between boards). It waits for the start
 *    marker frame that the sender (replay_sender) puts on the bus first,
 *    and starts its clock from that frame.
 *
 * 2. It keeps following the sender's clock for the whole run. The two
 *    crystals differ by a few ppm, which adds up to tens of microseconds over a
 *    few seconds: attack frames then land late and push the normal frames
 *    behind them aside. So the PC also sends this board a timing reference for
 *    every normal frame (when it should end on the bus). This board hears those
 *    frames, compares when they ended with when they should have, and shifts its
 *    own schedule by the difference. Frames held up behind other frames end
 *    late, so only the earliest one in each window counts.
 *
 * Both boards then share one time base within a few microseconds, so attack
 * frames land between the normal frames as in the car, and compete with them
 * in real arbitration.
 *
 * Serial protocol, 921600 baud, 8N1:
 *   board -> PC   "READY ATTACKER\n" once at boot
 *   PC -> board   15 byte packets, in time order:
 *                 attack frame:  id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                                t = microseconds after the START of the marker
 *                 reference:     0xFF id_hi id_lo 0 x 8 t0 t1 t2 t3
 *                                a normal frame the sender sends, t = when it ENDS
 *   board -> PC   one byte 'A' after every ACK_EVERY packets (both kinds)
 *   PC -> board   'Q'  ->  same "#TX ..." line as replay_sender, plus
 *                 synced=<0|1> tracked=<n> missed=<n> skew=<us> drift=<ppb>
 *
 * tracked counts normal frames matched to their reference, missed references
 * whose frame this board never heard, skew the final shift of the schedule
 * against the start and drift how fast it grew (parts per billion).
 *
 * Wiring: module TX -> GPIO 5, module RX -> GPIO 4. See docs/ESP32_WIRING.md
 */
#include "driver/twai.h"
#include "esp_timer.h"

#define PIN_CAN_TX  GPIO_NUM_5
#define PIN_CAN_RX  GPIO_NUM_4
#define ACK_EVERY   64
#define PACKET_LEN  15
#define REF_TAG     0xFF        // first byte of a reference packet (an ID byte is < 8)

// Start marker: ID 0x7FF with the bytes "CANSTART". On the wire it is 112 bits
// long, 224 us at 500 kbit/s (python/can_bits.py, stuff bits included). The
// controller reports a frame once it has ended, so the marker STARTED 224 us
// before that, which is the sender's time zero.
#define MARKER_ID   0x7FF
#define MARKER_US   224
// This board hears a frame later after it ends than replay_sender learns its
// own frame has gone out, so time zero taken from the marker comes out late.
// Measured on the bench (frames with a clear bus before them): 35 us late
// with no trim (run 20261012_225632), still 8 us late with 35 (20261012_230423).
#define SYNC_TRIM_US 43
static const uint8_t MARKER_DATA[8] = {'C', 'A', 'N', 'S', 'T', 'A', 'R', 'T'};

// Clock following. WINDOW_US of schedule time per estimate; the earliest
// arrival in a window is a frame nobody held up. The drift is measured from
// the first window, once at least DRIFT_AFTER_US have passed.
#define WINDOW_US       250000
#define MIN_IN_WINDOW   8
#define DRIFT_AFTER_US  1000000
#define MATCH_AHEAD     8           // references to look past for a frame this board missed
#define EXPIRE_US       20000       // a reference this far in the past: its frame was missed
#define REF_RING        2048        // power of two. When full, references wait in the USB buffer
#define RING            512         // power of two
#define REFS_PER_CALL   4           // keeps one service() call short
#define QUIET_US        50          // spin without other work this close to a send

struct Ref { uint16_t id; uint32_t end; };
struct Heard { uint16_t id; int64_t at; };
static Ref refs[REF_RING];
static Heard heard[RING];
static uint32_t ref_head = 0, ref_n = 0, heard_head = 0, heard_n = 0;

static int64_t t0 = -1;
static uint32_t processed = 0, tracked = 0, missed = 0;

// Schedule shift: skew(t) = anchor_skew + drift * (t - anchor_t), in us.
static bool have_base = false;
static float base_obs = 0, anchor_skew = 0, drift = 0;
static uint32_t base_t = 0, anchor_t = 0;
static bool win_open = false;
static uint32_t win_start = 0, win_count = 0, win_min_t = 0;
static float win_min = 0;

static float skew_at(uint32_t t)
{
    return anchor_skew + drift * (float)(int32_t)(t - anchor_t);   // subtract first: float has 24 bits
}

// One normal frame: it should have ended at `end`, it was heard at `at`.
static void track(uint32_t end, int64_t at)
{
    float obs = (float)(at - t0 - (int64_t)end);
    if (have_base && obs < base_obs + skew_at(end) - 100)
        return;                        // earlier than possible: a wrong match
    if (!win_open) {
        win_open = true;
        win_start = end;
        win_count = 0;
    }
    if (win_count == 0 || obs < win_min) {
        win_min = obs;
        win_min_t = end;
    }
    win_count++;
    if (end - win_start < WINDOW_US || win_count < MIN_IN_WINDOW)
        return;

    win_open = false;
    if (!have_base) {                  // the marker set the start, follow changes from here
        have_base = true;
        base_obs = win_min;
        base_t = anchor_t = win_min_t;
        return;
    }
    anchor_skew = win_min - base_obs;
    anchor_t = win_min_t;
    if (anchor_t - base_t >= DRIFT_AFTER_US)
        drift = anchor_skew / (float)(anchor_t - base_t);
}

static void ack_packet()
{
    if (++processed % ACK_EVERY == 0)
        Serial.write('A');
}

// Pair heard frames with references in order. A frame this board missed shows
// up as a reference with a different ID: skip it.
static void pair()
{
    int64_t too_old = esp_timer_get_time() - t0 - EXPIRE_US;
    while (ref_n && (int64_t)refs[ref_head].end < too_old) {
        ref_head = (ref_head + 1) & (REF_RING - 1);
        ref_n--;
        missed++;
    }
    while (ref_n && heard_n) {
        const Heard &h = heard[heard_head];
        uint32_t j = 0;
        while (j < ref_n && j < MATCH_AHEAD && refs[(ref_head + j) & (REF_RING - 1)].id != h.id)
            j++;
        if (j == ref_n && ref_n < MATCH_AHEAD)
            return;                    // its reference may not be here yet
        if (j == ref_n || j == MATCH_AHEAD) {
            heard_head = (heard_head + 1) & (RING - 1);    // not one of the sender's frames
            heard_n--;
            continue;
        }
        missed += j;
        ref_head = (ref_head + j) & (REF_RING - 1);
        ref_n -= j;
        track(refs[ref_head].end, h.at);
        tracked++;
        ref_head = (ref_head + 1) & (REF_RING - 1);
        ref_n--;
        heard_head = (heard_head + 1) & (RING - 1);
        heard_n--;
    }
}

static void take_ref(const uint8_t *pkt)
{
    Ref &r = refs[(ref_head + ref_n) & (REF_RING - 1)];
    r.id = (((uint16_t)pkt[1] << 8) | pkt[2]) & 0x7FF;
    r.end = (uint32_t)pkt[11] | ((uint32_t)pkt[12] << 8) |
            ((uint32_t)pkt[13] << 16) | ((uint32_t)pkt[14] << 24);
    ref_n++;
    ack_packet();
}

// Stamp every frame heard and read the references waiting at the head of the
// USB buffer (a few per call, and only while there is room: the rest wait
// there). Called often, so a frame is stamped soon after it ended.
static void service()
{
    twai_message_t m;
    while (twai_receive(&m, 0) == ESP_OK) {
        int64_t now = esp_timer_get_time();
        if (heard_n == RING) {
            heard_head = (heard_head + 1) & (RING - 1);
            heard_n--;
        }
        Heard &h = heard[(heard_head + heard_n) & (RING - 1)];
        h.id = m.identifier & 0x7FF;
        h.at = now;
        heard_n++;
    }
    static uint8_t pkt[PACKET_LEN];
    for (int k = 0; k < REFS_PER_CALL && ref_n < REF_RING &&
                    Serial.peek() == REF_TAG && Serial.available() >= PACKET_LEN; k++) {
        Serial.readBytes(pkt, PACKET_LEN);
        take_ref(pkt);
    }
    pair();
}

static void report(uint32_t sent, uint32_t late, uint32_t max_late, bool synced)
{
    twai_status_info_t s = {};
    twai_get_status_info(&s);
    const char *state = s.state == TWAI_STATE_RUNNING ? "running"
                      : s.state == TWAI_STATE_BUS_OFF ? "busoff"
                      : s.state == TWAI_STATE_RECOVERING ? "recovering" : "stopped";
    Serial.printf("#TX sent=%lu late=%lu maxlate=%lu tec=%lu rec=%lu state=%s arblost=%lu txfail=%lu buserr=%lu synced=%d"
                  " tracked=%lu missed=%lu skew=%ld drift=%ld\n",
                  (unsigned long)sent, (unsigned long)late, (unsigned long)max_late,
                  (unsigned long)s.tx_error_counter, (unsigned long)s.rx_error_counter, state,
                  (unsigned long)s.arb_lost_count, (unsigned long)s.tx_failed_count,
                  (unsigned long)s.bus_error_count, synced ? 1 : 0,
                  (unsigned long)tracked, (unsigned long)missed,
                  (long)lroundf(anchor_skew), (long)lroundf(drift * 1e9f));
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
                t0 = now - MARKER_US - SYNC_TRIM_US;
                break;
            }
        }
        return;
    }

    service();
    if (Serial.available() < PACKET_LEN || Serial.peek() == REF_TAG)
        return;
    Serial.readBytes(pkt, PACKET_LEN);

    uint32_t rel = (uint32_t)pkt[11] | ((uint32_t)pkt[12] << 8) |
                   ((uint32_t)pkt[13] << 16) | ((uint32_t)pkt[14] << 24);
    int64_t due;
    while (true) {
        due = t0 + (int64_t)rel + (int64_t)lroundf(skew_at(rel));
        int64_t now = esp_timer_get_time();
        if (now >= due)
            break;
        if (due - now > QUIET_US)
            service();
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
    sent++;
    ack_packet();
}
