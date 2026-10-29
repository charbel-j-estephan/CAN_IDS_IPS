/* CAN frame decoder, one bus bit at a time (0 = dominant, 1 = recessive).
 *
 * Plain C with no hardware access, so the same code runs in the STM32's bit
 * interrupt and in the PC test (test/host_test.c). The caller does the bit
 * timing: it calls can_rx_sof() on the falling edge that starts a frame and
 * can_rx_bit() at the sample point of every bit after that, until
 * can_rx_idle() says the bus is idle again (no sampling needed).
 *
 * It removes the stuff bits, checks the CRC-15 and the fixed form bits
 * (CRC delimiter, ACK delimiter, end of frame) and counts what went wrong.
 */
#ifndef CAN_RX_H
#define CAN_RX_H

#include <stdint.h>

/* -D CAN_RX_IN_RAM puts the decoder and the bit interrupt in RAM. Off by
 * default: the one build that ran from RAM decoded 0 of 300 frames (code
 * fetched from SRAM competes with the data and the peripherals on one bus),
 * against 247 of 300 from flash. */
#if defined(__arm__) && defined(CAN_RX_IN_RAM)
#define CAN_RX_FAST __attribute__((section(".RamFunc")))
#else
#define CAN_RX_FAST
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t id;          /* 11 bit (standard) or upper 11 bits (extended) */
    uint8_t dlc;          /* as sent, 0..15 */
    uint8_t ide, rtr;     /* extended frame, remote frame */
    uint8_t data[8];      /* min(dlc, 8) bytes, the rest 0 */
} can_frame_t;

typedef struct {
    uint32_t frames;      /* good frames of any kind */
    uint32_t stuff, form, crc;
} can_rx_stats_t;

extern can_rx_stats_t can_rx_stats;

/* 1: can_rx_bit() reports CAN_RX_FEATURES mid frame (default). 0: it never
 * does (diagnostic switch 'f' in main.cpp, to time the bit interrupt without
 * the mid frame copy). */
extern volatile uint8_t can_rx_features_on;

/* Wait for 11 recessive bits before taking a start of frame (power up). */
void can_rx_reset(void);
/* The bus is idle: stop sampling, the next falling edge is a start of frame. */
int can_rx_idle(void);
/* A falling edge while idle: a frame starts with this bit. The start of frame
 * bit itself is dominant by definition; the caller passes it to can_rx_bit()
 * as 0 instead of sampling it, so a late first interrupt cannot misread it. */
void can_rx_sof(void);
/* What can_rx_bit() returns (0: nothing to report). */
#define CAN_RX_FRAME          1   /* a good frame is complete, at the 6th end of
                                     frame bit, where CAN counts it as received */
#define CAN_RX_FEATURES       2   /* the IDS features are in: ID, DLC and the
                                     first CAN_RX_FEATURE_BYTES data bytes (all
                                     the data if there are fewer). Later bytes
                                     are 0. The frame is not checked yet. */
#define CAN_RX_FEATURE_BYTES  5

/* One sampled bit. *out is filled for CAN_RX_FRAME and CAN_RX_FEATURES. */
int can_rx_bit(uint8_t level, can_frame_t *out);

#ifdef __cplusplus
}
#endif

#endif
