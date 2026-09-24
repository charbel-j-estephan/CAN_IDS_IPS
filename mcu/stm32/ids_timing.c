/* Time the random forest on the STM32F407 with the DWT cycle counter.
 *
 * Add this file, ids_timing.h and generated/can_ids_model.h to your
 * STM32CubeIDE project. Call ids_timing_init() once after SystemClock_Config(),
 * then ids_classify() for every frame HAL_CAN_GetRxMessage() gives you.
 *
 * The DWT counter ticks once per CPU clock (168 MHz on the F407 when set up
 * with CubeMX defaults), so it measures to about 6 ns with no timer setup.
 */
#include "ids_timing.h"

#include "stm32f4xx.h"
/* The generated header defines functions, so include it in this one file only. */
#include "can_ids_model.h"

void ids_timing_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

ids_result_t ids_classify(uint32_t can_id, uint8_t dlc, const uint8_t data[8])
{
    /* Same order as python/dataset.py FEATURES. Bytes past the DLC must be 0. */
    float features[10];
    features[0] = (float)(can_id & 0x7FF);
    features[1] = (float)(dlc > 8 ? 8 : dlc);
    for (int i = 0; i < 8; i++)
        features[2 + i] = i < dlc ? (float)data[i] : 0.0f;

    ids_result_t r;
    uint32_t start = DWT->CYCCNT;
    r.class_index = can_ids_predict(features, 10);
    r.cycles = DWT->CYCCNT - start;
    return r;
}

float ids_cycles_to_us(uint32_t cycles)
{
    return (float)cycles * 1e6f / (float)SystemCoreClock;
}

/* Example use inside the CAN receive callback:
 *
 *   void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
 *   {
 *       CAN_RxHeaderTypeDef hdr;
 *       uint8_t data[8] = {0};
 *       HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data);
 *       ids_result_t r = ids_classify(hdr.StdId, hdr.DLC, data);
 *       // log r.class_index and ids_cycles_to_us(r.cycles) over UART
 *   }
 *
 * Record the worst case over many frames, not the average. Deep trees make
 * some frames slower than others, and the budget applies to every frame.
 */
