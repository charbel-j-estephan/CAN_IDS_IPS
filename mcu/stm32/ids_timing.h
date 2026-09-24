#ifndef IDS_TIMING_H
#define IDS_TIMING_H

#include <stdint.h>

/* Classify one CAN frame and report how many CPU cycles it took. */
typedef struct {
    int32_t class_index;  /* 0 Normal, 1 DoS, 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof */
    uint32_t cycles;      /* CPU cycles spent in can_ids_predict() */
} ids_result_t;

void ids_timing_init(void);
ids_result_t ids_classify(uint32_t can_id, uint8_t dlc, const uint8_t data[8]);
float ids_cycles_to_us(uint32_t cycles);

#endif
