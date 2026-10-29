/* The IDS model on the STM32, see classify.h. can_ids_model.h defines
 * can_ids_predict() (not static), so it is included here only.
 *
 * Build with -D MODEL_IN_RAM to run the model from RAM (.RamFunc) instead of
 * flash. fw 6 did that, to keep the model out of the flash cache the bit
 * interrupt uses; it did not fix the late bit samples (they came with the
 * model switched off as well), so flash is the default again. */
#include "classify.h"

#ifdef MODEL_IN_RAM
#define MODEL_PLACE __attribute__((section(".RamFunc")))
#define CAN_IDS_FN static inline MODEL_PLACE
#define CAN_IDS_PUBLIC MODEL_PLACE
#else
#define MODEL_PLACE
#endif
#include "can_ids_model.h"

const char *const classify_model_id = CAN_IDS_MODEL_ID;

MODEL_PLACE int classify(uint16_t can_id, uint8_t dlc, const uint8_t d[5])
{
    /* Features in dataset.FEATURES order: ID, DLC (at most 8), data bytes 0-7.
     * The model reads bytes 0-4; bytes 5-7 are 0, as in training. */
    float x[10] = {(float)can_id, (float)(dlc > 8 ? 8 : dlc),
                   (float)d[0], (float)d[1], (float)d[2], (float)d[3], (float)d[4],
                   0.0f, 0.0f, 0.0f};
    int cls = (int)can_ids_predict(x, 10);
    if (cls == 0 && !can_ids_id_known(can_id))   /* the unknown ID rule */
        cls = 2;                                  /* Fuzzy */
    return cls;
}
