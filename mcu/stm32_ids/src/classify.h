/* The IDS model on the STM32: the random forest from python/train.py
 * (exported by python/export_c.py into can_ids_model.h) plus the unknown ID
 * rule (python/forest.py known_id_rule). Same answer as Python for every
 * frame: python/check_stm32_model.py checks it on a recorded run. */
#ifndef CLASSIFY_H
#define CLASSIFY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 0 Normal, 1 DoS, 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof. d holds the first 5
 * data bytes, 0 past the DLC (as dataset.py and train.py --payload-bytes 5). */
int classify(uint16_t can_id, uint8_t dlc, const uint8_t d[5]);

extern const char *const classify_model_id;

#ifdef __cplusplus
}
#endif

#endif
