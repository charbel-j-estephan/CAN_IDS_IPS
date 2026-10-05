/* STM32F407 CAN gateway: forwards every frame from bus 1 to bus 2, unchanged.
 *
 *   ESP32 A --- bus 1 --- [ CAN1 -> CAN2 ] --- bus 2 --- ESP32 B
 *
 * Pins (each CAN port needs its own transceiver module):
 *   CAN1  RX PB8   TX PB9     bus 1, towards the sender
 *   CAN2  RX PB12  TX PB13    bus 2, towards the receiver
 * PA11/PA12 (the usual CAN1 pins) are the USB lines on this board.
 *
 * Frames wait in a 512 frame queue if bus 2 is busy, so nothing is lost on
 * short bursts. Once a second the board prints over the mini-USB (it shows up
 * as a COM port; any baud rate):
 *   #GW rx=<frames in> fwd=<frames out> drop=<queue full> qmax=<deepest queue>
 *       proc_max_us=<slowest read-to-send time>
 *
 * LEDs (active low): D2 (PA6) flickers while forwarding, D3 (PA7) lights if a
 * frame was ever dropped. Both blink together if CAN setup failed.
 */
#include <Arduino.h>

#define LED_FWD   PA6
#define LED_DROP  PA7
#define QUEUE     512

static CAN_HandleTypeDef hcan1;
static CAN_HandleTypeDef hcan2;

struct Frame {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t t_in;                 // cycle counter when it was read from CAN1
};
static Frame queue_[QUEUE];
static uint32_t q_head = 0, q_tail = 0;

static uint32_t n_rx = 0, n_fwd = 0, n_drop = 0, q_max = 0, proc_max = 0;

static void fail()
{
    for (;;) {
        digitalWrite(LED_FWD, LOW);
        digitalWrite(LED_DROP, LOW);
        delay(150);
        digitalWrite(LED_FWD, HIGH);
        digitalWrite(LED_DROP, HIGH);
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

    __HAL_RCC_CAN1_CLK_ENABLE();       // CAN2 only works with CAN1's clock on too
    __HAL_RCC_CAN2_CLK_ENABLE();
}

static void can_begin(CAN_HandleTypeDef *h, CAN_TypeDef *inst, uint32_t prescaler)
{
    h->Instance = inst;
    h->Init.Prescaler = prescaler;               // 500 kbit/s: 14 time quanta per bit
    h->Init.Mode = CAN_MODE_NORMAL;
    h->Init.SyncJumpWidth = CAN_SJW_1TQ;
    h->Init.TimeSeg1 = CAN_BS1_11TQ;
    h->Init.TimeSeg2 = CAN_BS2_2TQ;              // sample point at 12 of 14 = 86 %
    h->Init.TimeTriggeredMode = DISABLE;
    h->Init.AutoBusOff = ENABLE;
    h->Init.AutoWakeUp = DISABLE;
    h->Init.AutoRetransmission = ENABLE;
    h->Init.ReceiveFifoLocked = DISABLE;
    h->Init.TransmitFifoPriority = ENABLE;       // send in the order received
    if (HAL_CAN_Init(h) != HAL_OK)
        fail();
}

void setup()
{
    pinMode(LED_FWD, OUTPUT);
    pinMode(LED_DROP, OUTPUT);
    digitalWrite(LED_FWD, HIGH);
    digitalWrite(LED_DROP, HIGH);
    SerialUSB.begin(115200);

    // Cycle counter, to time how long each frame spends inside the gateway.
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Bit rate = APB1 clock / prescaler / 14. The prescaler must divide evenly.
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t prescaler = pclk1 / (500000UL * 14UL);
    if (prescaler == 0 || prescaler * 500000UL * 14UL != pclk1)
        fail();

    pins_begin();
    can_begin(&hcan1, CAN1, prescaler);
    can_begin(&hcan2, CAN2, prescaler);

    CAN_FilterTypeDef f = {};                    // CAN1 accepts every frame into FIFO 0
    f.FilterBank = 0;
    f.FilterMode = CAN_FILTERMODE_IDMASK;
    f.FilterScale = CAN_FILTERSCALE_32BIT;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation = ENABLE;
    f.SlaveStartFilterBank = 14;                 // banks 14 and up belong to CAN2
    if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
        fail();

    if (HAL_CAN_Start(&hcan1) != HAL_OK || HAL_CAN_Start(&hcan2) != HAL_OK)
        fail();
}

static void receive()
{
    while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
        CAN_RxHeaderTypeDef rh;
        uint8_t d[8];
        if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rh, d) != HAL_OK)
            return;
        uint32_t t = DWT->CYCCNT;
        if (rh.IDE != CAN_ID_STD || rh.RTR != CAN_RTR_DATA)
            continue;
        n_rx++;
        if (q_head - q_tail >= QUEUE) {
            n_drop++;
            digitalWrite(LED_DROP, LOW);
            continue;
        }
        Frame &fr = queue_[q_head % QUEUE];
        fr.id = rh.StdId;
        fr.dlc = rh.DLC > 8 ? 8 : rh.DLC;
        memcpy(fr.data, d, 8);
        fr.t_in = t;
        q_head++;
        if (q_head - q_tail > q_max)
            q_max = q_head - q_tail;
    }
}

static void transmit()
{
    while (q_tail != q_head && HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) > 0) {
        Frame &fr = queue_[q_tail % QUEUE];
        CAN_TxHeaderTypeDef th = {};
        th.StdId = fr.id;
        th.IDE = CAN_ID_STD;
        th.RTR = CAN_RTR_DATA;
        th.DLC = fr.dlc;
        th.TransmitGlobalTime = DISABLE;
        uint32_t mailbox;
        if (HAL_CAN_AddTxMessage(&hcan2, &th, fr.data, &mailbox) != HAL_OK)
            return;
        uint32_t us = (DWT->CYCCNT - fr.t_in) / (SystemCoreClock / 1000000);
        if (us > proc_max)
            proc_max = us;
        q_tail++;
        if ((++n_fwd & 0xFF) == 0)
            digitalToggle(LED_FWD);
    }
}

void loop()
{
    receive();
    transmit();

    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last >= 1000) {
        last = now;
        char line[120];
        int n = snprintf(line, sizeof line, "#GW rx=%lu fwd=%lu drop=%lu qmax=%lu proc_max_us=%lu\n",
                         (unsigned long)n_rx, (unsigned long)n_fwd, (unsigned long)n_drop,
                         (unsigned long)q_max, (unsigned long)proc_max);
        // Never wait for USB: if nothing on the PC is reading, skip the line
        // rather than stall forwarding.
        if (n > 0 && SerialUSB.availableForWrite() >= n)
            SerialUSB.write((const uint8_t *)line, n);
    }
}
