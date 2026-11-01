/* The IDS model on the STM32, see classify.h. can_ids_model.h defines
 * can_ids_predict() (not static), so it is included here only.
 *
 * Build with -D MODEL_IN_RAM to run the model from RAM (.RamFunc) instead of
 * flash, which keeps the model out of the flash cache the bit interrupt runs
 * from. On by default again since fw 11 (platformio.ini): fw 6 tried it while
 * the bit interrupt itself was too heavy; once that was fixed, fw 10 with the
 * model in flash still sampled up to 130 ticks late (fw 9, no model: 88). */
#include "classify.h"

#ifdef MODEL_IN_RAM
#define MODEL_PLACE __attribute__((section(".RamFunc")))
#define CAN_IDS_FN static inline MODEL_PLACE
#define CAN_IDS_PUBLIC MODEL_PLACE
#else
#define MODEL_PLACE
#endif
/* One model per byte count, all from python/train.py --trees 5 --max-depth 9
 * --payload-bytes N, exported by python/export_c.py. */
#if CAN_RX_FEATURE_BYTES == 5
#include "can_ids_model.h"           /* rf5-d9-p5-2ddda531, the original */
#elif CAN_RX_FEATURE_BYTES == 6
#include "can_ids_model_p6.h"
#elif CAN_RX_FEATURE_BYTES == 8
#include "can_ids_model_p8.h"
#else
#error "no model for this CAN_RX_FEATURE_BYTES"
#endif

const char *const classify_model_id = CAN_IDS_MODEL_ID;

MODEL_PLACE int classify(uint16_t can_id, uint8_t dlc, const uint8_t d[CAN_RX_FEATURE_BYTES])
{
    /* Features in dataset.FEATURES order: ID, DLC (at most 8), data bytes 0-7.
     * The model reads the first CAN_RX_FEATURE_BYTES bytes; the rest are 0,
     * as in training. */
#if CAN_RX_FEATURE_BYTES == 5
    /* written out as in fw 16, so the 5-byte firmware stays byte for byte the same */
    float x[10] = {(float)can_id, (float)(dlc > 8 ? 8 : dlc),
                   (float)d[0], (float)d[1], (float)d[2], (float)d[3], (float)d[4],
                   0.0f, 0.0f, 0.0f};
#else
    float x[10] = {(float)can_id, (float)(dlc > 8 ? 8 : dlc),
                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    for (int i = 0; i < CAN_RX_FEATURE_BYTES; i++)
        x[2 + i] = (float)d[i];
#endif
    int cls = (int)can_ids_predict(x, 10);
    if (cls == 0 && !can_ids_id_known(can_id))   /* the unknown ID rule */
        cls = 2;                                  /* Fuzzy */
    return cls;
}
