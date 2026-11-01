/* CAN frame decoder, see can_rx.h. */
#include "can_rx.h"

#define CRC15_POLY  0x4599

enum { IDLE, IN_FRAME, TAIL, WAIT_IDLE };

can_rx_stats_t can_rx_stats;
volatile uint8_t can_rx_features_on = 1;
volatile uint8_t can_rx_quiet = 0;

static uint8_t state = WAIT_IDLE;
static uint16_t nbits;          /* bits received after destuffing, start of frame is bit 0 */
static uint16_t frame_end;      /* bit count when the CRC field is complete */
static uint16_t feat_end;       /* bit count when the IDS features are complete */
static uint16_t crc;            /* CRC-15 over all bits: 0 at the end of a good frame */
static uint8_t last, run;       /* last bit and how many in a row, for destuffing */
static uint8_t stuff_next;      /* the next bit on the wire is a stuff bit */
static uint8_t tail;            /* position after the CRC field */
static uint16_t rec_run;        /* recessive bits in a row while waiting for an idle bus */
static can_frame_t f;

/* Something was wrong with this frame: ignore the bus until it has been
 * recessive for 11 bits (the error frame and the gap after it). */
CAN_RX_FAST static void give_up(uint32_t *counter)
{
    if (counter && !can_rx_quiet)
        (*counter)++;
    state = WAIT_IDLE;
    rec_run = 0;
}

void can_rx_reset(void)
{
    give_up(0);
}

CAN_RX_FAST int can_rx_idle(void)
{
    return state == IDLE;
}

/* Still in time for an error flag that destroys the frame for every node:
 * anywhere before the end of frame field (TAIL: 0 CRC delimiter, 1 ACK slot,
 * 2 ACK delimiter, 3.. end of frame). */
CAN_RX_FAST int can_rx_before_eof(void)
{
    return state == IN_FRAME || (state == TAIL && tail < 3);
}

CAN_RX_FAST void can_rx_sof(void)
{
    int i;
    nbits = 0;
    frame_end = 0xFFFF;
    feat_end = 0xFFFF;
    crc = 0;
    run = 0;
    stuff_next = 0;
    f.id = 0;
    f.dlc = f.ide = f.rtr = 0;
    for (i = 0; i < 8; i++)
        f.data[i] = 0;
    state = IN_FRAME;
}

CAN_RX_FAST static void data_bit(uint8_t level)
{
    uint16_t i = nbits, top, dlc_at;

    top = (crc >> 14) & 1;
    crc = (crc << 1) & 0x7FFF;
    if (top ^ level)
        crc ^= CRC15_POLY;

    /* Standard: SOF, ID 1-11, RTR 12, IDE 13, r0 14, DLC 15-18, data 19+.
     * Extended: SOF, ID 1-11, SRR 12, IDE 13, ID 14-31, RTR 32, r1 r0, DLC 35-38, data 39+. */
    dlc_at = f.ide ? 35 : 15;
    if (i >= 1 && i <= 11) {
        f.id = (uint16_t)((f.id << 1) | level);
    } else if (i == 12) {
        f.rtr = level;
    } else if (i == 13) {
        f.ide = level;
    } else if (f.ide && i == 32) {
        f.rtr = level;
    } else if (i >= dlc_at && i < dlc_at + 4) {
        f.dlc = (uint8_t)((f.dlc << 1) | level);
        if (i == dlc_at + 3) {
            uint16_t n = f.rtr ? 0 : (f.dlc > 8 ? 8 : f.dlc);
            frame_end = dlc_at + 4 + 8 * n + 15;
            if (can_rx_features_on)     /* else feat_end stays 0xFFFF: never reported */
                feat_end = dlc_at + 4 + 8 * (n > CAN_RX_FEATURE_BYTES ? CAN_RX_FEATURE_BYTES : n);
        }
    } else if (frame_end != 0xFFFF && i >= dlc_at + 4 && i < frame_end - 15) {
        uint16_t k = (uint16_t)((i - dlc_at - 4) >> 3);
        f.data[k] = (uint8_t)((f.data[k] << 1) | level);
    }
    nbits++;
}

CAN_RX_FAST int can_rx_bit(uint8_t level, can_frame_t *out)
{
    switch (state) {
    case WAIT_IDLE:
        rec_run = level ? rec_run + 1 : 0;
        if (rec_run >= 11) {
            state = IDLE;
            return CAN_RX_IDLE;
        }
        return 0;

    case IN_FRAME:
        if (stuff_next) {               /* after five equal bits comes the opposite one */
            if (level == last) {
                give_up(&can_rx_stats.stuff);
                return 0;
            }
            stuff_next = 0;
            last = level;
            run = 1;
            if (nbits == frame_end) {
                state = TAIL;
                tail = 0;
            }
            return 0;
        }
        if (nbits == 0 && level) {      /* no dominant start of frame: a glitch */
            state = IDLE;
            return CAN_RX_IDLE;
        }
        if (nbits > 0 && level == last) {
            run++;
        } else {
            last = level;
            run = 1;
        }
        data_bit(level);
        if (run == 5) {
            stuff_next = 1;             /* also after the last CRC bit */
        } else if (nbits == frame_end) {
            state = TAIL;
            tail = 0;
        }
        if (nbits == feat_end) {        /* ID, DLC and the first data bytes are in */
            *out = f;
            return CAN_RX_FEATURES;
        }
        return 0;

    case TAIL:
        /* tail 0: CRC delimiter, 1: ACK slot, 2: ACK delimiter, 3-9: end of frame */
        if (tail == 0 && crc != 0) {
            give_up(&can_rx_stats.crc);
            return 0;
        }
        if (!level && tail != 1) {
            if (tail == 9)              /* last end of frame bit: overload, the frame stands */
                give_up(0);
            else
                give_up(&can_rx_stats.form);
            return 0;
        }
        tail++;
        if (tail == 9) {                /* 6th end of frame bit done: received */
            can_rx_stats.frames++;
            *out = f;
            return CAN_RX_FRAME;
        }
        if (tail == 10) {
            state = IDLE;
            return CAN_RX_IDLE;
        }
        return 0;

    default:                            /* IDLE: nothing to sample, stop */
        return CAN_RX_IDLE;
    }
}
