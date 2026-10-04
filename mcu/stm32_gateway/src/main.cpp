/* STM32F407 CAN gateway with the random forest in the middle.
 *
 *   ESP32 A --- bus 1 --- [ CAN1 -> forest -> CAN2 ] --- bus 2 --- ESP32 B
 *
 * Every frame arriving on CAN1 is classified. Normal frames are forwarded on
 * CAN2. Attack frames are dropped when BLOCK_ATTACKS is 1 (intrusion
 * PREVENTION), or forwarded anyway when it is 0 (detection only, the LED
 * still shows what the model thought).
 *
 * Pins (CAN1 and CAN2 each need a transceiver module):
 *   CAN1  RX PB8   TX PB9     bus 1, towards the sender
 *   CAN2  RX PB12  TX PB13    bus 2, towards the receiver
 * The default CAN1 pins PA11/PA12 are the USB data lines on this board, so
 * they are not used.
 *
 * LEDs (active low): D2 on PA6 flickers as frames are forwarded, D3 on PA7
 * flickers as frames are blocked. Both blink together if setup fails: the
 * bus clock is not what the bit timing expects.
 *
 * Needs can_ids_model.h in this folder: run python/export_c.py, then
 * python python/esp32_eval.py --copy-model.
 */
#include <Arduino.h>

#include "can_ids_model.h"

#define BLOCK_ATTACKS 1
#define LED_PASS      PA6
#define LED_BLOCK     PA7

static CAN_HandleTypeDef hcan1;
static CAN_HandleTypeDef hcan2;

static uint32_t n_pass = 0, n_block = 0, n_drop = 0;

static void fail()
{
    for (;;) {
        digitalWrite(LED_PASS, LOW);
        digitalWrite(LED_BLOCK, LOW);
        delay(150);
        digitalWrite(LED_PASS, HIGH);
        digitalWrite(LED_BLOCK, HIGH);
        delay(150);
    }
}

static void pins_begin()
{
    GPIO_InitTypeDef g = {};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    g.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    g.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &g);
    g.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    g.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &g);

    // CAN2 only works when CAN1's clock is on as well.
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();
}

static void can_begin(CAN_HandleTypeDef *h, CAN_TypeDef *inst, uint32_t prescaler)
{
    h->Instance = inst;
    h->Init.Prescaler = prescaler;               // 500 kbit/s: 14 time quanta per bit
    h->Init.Mode = CAN_MODE_NORMAL;              // normal, so both nodes ACK frames
    h->Init.SyncJumpWidth = CAN_SJW_1TQ;
    h->Init.TimeSeg1 = CAN_BS1_11TQ;
    h->Init.TimeSeg2 = CAN_BS2_2TQ;              // sample point at 12 of 14 = 86 %
    h->Init.TimeTriggeredMode = DISABLE;
    h->Init.AutoBusOff = ENABLE;
    h->Init.AutoWakeUp = DISABLE;
    h->Init.AutoRetransmission = ENABLE;
    h->Init.ReceiveFifoLocked = DISABLE;
    h->Init.TransmitFifoPriority = ENABLE;       // keep frames in order
    if (HAL_CAN_Init(h) != HAL_OK)
        fail();
}

void setup()
{
    pinMode(LED_PASS, OUTPUT);
    pinMode(LED_BLOCK, OUTPUT);
    digitalWrite(LED_PASS, HIGH);
    digitalWrite(LED_BLOCK, HIGH);

    // Bit rate = APB1 clock / prescaler / 14. The prescaler must divide evenly.
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t prescaler = pclk1 / (500000UL * 14UL);
    if (prescaler == 0 || prescaler * 500000UL * 14UL != pclk1)
        fail();

    pins_begin();
    can_begin(&hcan1, CAN1, prescaler);
    can_begin(&hcan2, CAN2, prescaler);

    // CAN1 accepts every frame into FIFO 0. Filter banks 14 and up belong to CAN2.
    CAN_FilterTypeDef f = {};
    f.FilterBank = 0;
    f.FilterMode = CAN_FILTERMODE_IDMASK;
    f.FilterScale = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh = 0;
    f.FilterIdLow = 0;
    f.FilterMaskIdHigh = 0;
    f.FilterMaskIdLow = 0;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation = ENABLE;
    f.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
        fail();

    if (HAL_CAN_Start(&hcan1) != HAL_OK || HAL_CAN_Start(&hcan2) != HAL_OK)
        fail();
}

void loop()
{
    while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
        CAN_RxHeaderTypeDef rh;
        uint8_t d[8] = {0};
        if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rh, d) != HAL_OK)
            break;
        if (rh.IDE != CAN_ID_STD || rh.RTR != CAN_RTR_DATA)
            continue;

        float feat[10];
        uint32_t dlc = rh.DLC > 8 ? 8 : rh.DLC;
        feat[0] = (float)(rh.StdId & 0x7FF);
        feat[1] = (float)dlc;
        for (uint32_t i = 0; i < 8; i++)
            feat[2 + i] = (i < dlc) ? (float)d[i] : 0.0f;   // past the DLC is 0, as in training

        int32_t cls = can_ids_predict(feat, 10);

        if (cls != 0 && BLOCK_ATTACKS) {
            if ((++n_block & 0xFF) == 0)
                digitalToggle(LED_BLOCK);
            continue;
        }

        CAN_TxHeaderTypeDef th = {};
        th.StdId = rh.StdId;
        th.IDE = CAN_ID_STD;
        th.RTR = CAN_RTR_DATA;
        th.DLC = dlc;
        th.TransmitGlobalTime = DISABLE;

        uint32_t mailbox;
        uint32_t t0 = millis();
        while (HAL_CAN_AddTxMessage(&hcan2, &th, d, &mailbox) != HAL_OK) {
            if (millis() - t0 > 50) {   // bus 2 stuck, for example nobody ACKs
                n_drop++;
                break;
            }
        }
        if ((++n_pass & 0xFF) == 0)
            digitalToggle(LED_PASS);
    }
}
