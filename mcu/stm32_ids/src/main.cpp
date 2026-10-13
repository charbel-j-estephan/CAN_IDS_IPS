/* STM32 IDS node, M1: receives CAN frames in software, bit by bit.
 *
 * Black STM32F407VE with a TJA1050 module (docs/STM32_WIRING.md):
 *   module RX -> PB8   read here (5 V tolerant pin)
 *   module TX -> PB9   open drain, released: the module holds it recessive,
 *                      so this board never touches the bus (M3 will)
 *
 * No CAN controller is used. TIM4 does the bit timing at 500 kbit/s:
 *   - channel 3 captures every falling edge on PB8 (recessive to dominant) in
 *     hardware. The edge of a start of frame starts a frame (hard sync), every
 *     other one moves the sample point (resync), as a CAN controller does.
 *   - channel 1 interrupts at the sample point of every bit, half a bit after
 *     the last edge, and the interrupt reads PB8.
 * can_rx.c turns the bits into frames (destuffing, CRC-15, form checks); the
 * interrupt puts each good frame in a buffer and loop() sends it to the PC.
 *
 * To the PC it looks like the receiver (usb_receiver), so
 * python/replay_dataset.py --receiver <this COM port> checks every frame:
 *   board -> PC   "READY STM32\n" at boot and whenever the PC opens the port
 *                 (the RTS pulse the scripts use to restart a board)
 *   board -> PC   frame record, 16 bytes: 0xA7 id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                 t = microseconds (TIM2), stamped at the 6th end of frame bit,
 *                 where CAN counts the frame as received
 *   board -> PC   once a second: "#STAT rx=<frames sent to the PC> ovf=<frames
 *                 lost, buffer full> drop=0 peak=<most frames waiting>
 *                 buserr=<frames with errors> rec=0 stuff=<n> form=<n> crc=<n>
 *                 other=<extended or remote frames, not sent>\n"
 *
 * Only standard data frames are sent to the PC, as the receiver does.
 */
#include <Arduino.h>
#include "can_rx.h"

#define BITRATE     500000
#define RING        512          // frames waiting for USB, power of two
#define LED         PA6          // on (low) while frames arrive

struct Frame {
    uint32_t ts;
    can_frame_t f;
};

static Frame ring[RING];
static volatile uint32_t ring_head = 0, ring_tail = 0;   // the interrupt moves head, loop() tail
static volatile uint32_t n_ovf = 0, n_other = 0;
static uint32_t n_sent = 0, peak = 0;

static uint16_t bit_ticks, sample_ticks;                 // TIM4 ticks

// ---- bit timing (TIM4 interrupt) -----------------------------------------------------

// Next sample at TIM4 tick `at`. If that moment has already passed (a late
// interrupt), sample at once instead of waiting for the 16 bit counter to wrap.
static inline void sample_at(uint16_t at)
{
    TIM4->CCR1 = at;
    if ((int16_t)(TIM4->CNT - at) >= 0)
        TIM4->EGR = TIM_EGR_CC1G;
}

static inline void sampling(bool on)
{
    if (on) {
        TIM4->SR = ~TIM_SR_CC1IF;
        TIM4->DIER |= TIM_DIER_CC1IE;
    } else {
        TIM4->DIER &= ~TIM_DIER_CC1IE;
    }
}

static void keep(const can_frame_t &f)
{
    if (f.ide || f.rtr) {
        n_other++;
        return;
    }
    uint32_t next = (ring_head + 1) & (RING - 1);
    if (next == ring_tail) {
        n_ovf++;
        return;
    }
    ring[ring_head].ts = TIM2->CNT;
    ring[ring_head].f = f;
    ring_head = next;
}

extern "C" void TIM4_IRQHandler(void)
{
    uint32_t sr = TIM4->SR;
    if ((sr & TIM_SR_CC1IF) && (TIM4->DIER & TIM_DIER_CC1IE)) {
        uint8_t level = (GPIOB->IDR >> 8) & 1;
        TIM4->SR = ~TIM_SR_CC1IF;
        sample_at(TIM4->CCR1 + bit_ticks);
        can_frame_t f;
        if (can_rx_bit(level, &f))
            keep(f);
        if (can_rx_idle())
            sampling(false);
    }
    if (sr & TIM_SR_CC3IF) {
        uint16_t at = TIM4->CCR3;          // reading it clears the flag
        TIM4->SR = ~TIM_SR_CC3OF;
        if (can_rx_idle()) {               // start of frame: hard sync
            can_rx_sof();
            sampling(true);
        }                                  // otherwise resync: the sample point follows the edge
        sample_at(at + sample_ticks);
    }
}

// ---- setup ---------------------------------------------------------------------------

static void start_timers()
{
    // TIM2 and TIM4 run from the APB1 timer clock (84 MHz on this board).
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t tclk = (RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1 ? pclk1 : 2 * pclk1;
    bit_ticks = tclk / BITRATE;
    sample_ticks = bit_ticks / 2;          // half a bit after the edge, room for interrupt latency

    // TIM2: free running microsecond clock for the frame time stamps.
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->CR1 = 0;
    TIM2->PSC = tclk / 1000000 - 1;
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_CEN;

    // PB8 as TIM4 channel 3 input. Open drain: this pin never drives 3.3 V
    // against the module's 5 V output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {};
    g.Pin = GPIO_PIN_8;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOB, &g);

    // TIM4: channel 3 captures falling edges (8 sample filter against
    // glitches), channel 1 is a plain compare for the sample points.
    __HAL_RCC_TIM4_CLK_ENABLE();
    TIM4->CR1 = 0;
    TIM4->PSC = 0;
    TIM4->ARR = 0xFFFF;
    TIM4->CCMR1 = 0;
    TIM4->CCMR2 = TIM_CCMR2_CC3S_0 | (3 << TIM_CCMR2_IC3F_Pos);
    TIM4->CCER = TIM_CCER_CC3E | TIM_CCER_CC3P;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR = 0;
    TIM4->DIER = TIM_DIER_CC3IE;
    NVIC_SetPriority(TIM4_IRQn, 0);        // above USB (1) and SysTick (1, platformio.ini):
                                           // a late sample reads the wrong bit
    NVIC_EnableIRQ(TIM4_IRQn);
    TIM4->CR1 = TIM_CR1_CEN;

    // Start by sampling until the bus has been idle for 11 bits.
    can_rx_reset();
    sampling(true);
    sample_at(TIM4->CNT + bit_ticks);
}

void setup()
{
    // CAN TX released (recessive). High is written before the pin becomes an
    // output, so it is never low for a moment: that would put a dominant bit
    // on the bus and could break a frame from another node.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIOB->BSRR = GPIO_PIN_9;
    GPIO_InitTypeDef tx = {};
    tx.Pin = GPIO_PIN_9;
    tx.Mode = GPIO_MODE_OUTPUT_OD;
    tx.Pull = GPIO_NOPULL;
    tx.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &tx);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    Serial.begin(921600);                  // USB: the baud rate is ignored
    Serial.dtr(false);                     // send even though the scripts hold DTR low
    start_timers();
    Serial.print("READY STM32\n");
}

// ---- loop: frames to the PC ----------------------------------------------------------

void loop()
{
    static bool rts_was = false;
    static uint32_t last_stat = 0, last_frame = 0;

    bool rts = Serial.rts();               // the scripts pulse RTS to restart a board
    if (rts_was && !rts)
        Serial.print("READY STM32\n");
    rts_was = rts;

    uint32_t waiting = (ring_head - ring_tail) & (RING - 1);
    if (waiting > peak)
        peak = waiting;
    while (ring_tail != ring_head && Serial.availableForWrite() >= 16) {
        const Frame &e = ring[ring_tail];
        uint8_t r[16];
        r[0] = 0xA7;
        r[1] = (e.f.id >> 8) & 0x07;
        r[2] = e.f.id & 0xFF;
        r[3] = e.f.dlc > 8 ? 8 : e.f.dlc;
        memcpy(&r[4], e.f.data, 8);
        r[12] = e.ts;
        r[13] = e.ts >> 8;
        r[14] = e.ts >> 16;
        r[15] = e.ts >> 24;
        ring_tail = (ring_tail + 1) & (RING - 1);
        Serial.write(r, sizeof r);
        n_sent++;
        last_frame = millis();
    }

    uint32_t now = millis();
    digitalWrite(LED, now - last_frame < 50 ? LOW : HIGH);
    if (now - last_stat >= 1000) {
        last_stat = now;
        const can_rx_stats_t &s = can_rx_stats;
        Serial.printf("#STAT rx=%lu ovf=%lu drop=0 peak=%lu buserr=%lu rec=0 stuff=%lu form=%lu crc=%lu other=%lu\n",
                      (unsigned long)n_sent, (unsigned long)n_ovf, (unsigned long)peak,
                      (unsigned long)(s.stuff + s.form + s.crc), (unsigned long)s.stuff,
                      (unsigned long)s.form, (unsigned long)s.crc, (unsigned long)n_other);
    }
}
