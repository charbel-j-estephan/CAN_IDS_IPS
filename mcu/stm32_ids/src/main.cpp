/* STM32 IDS node, M1: receives CAN frames in software, bit by bit.
 *
 * Black STM32F407VE with a TJA1050 module (docs/STM32_WIRING.md):
 *   module RX -> PB8   read here (5 V tolerant pin)
 *   module TX -> PB9   open drain, released: the module holds it recessive,
 *                      so this board never touches the bus (M3 will)
 *
 * No CAN controller is used. TIM4 does the bit timing at 500 kbit/s:
 *   - channel 3 captures every falling edge on PB8 (recessive to dominant) in
 *     hardware. On an idle bus it interrupts: that edge starts a frame (hard
 *     sync). Inside a frame it does not; the sample interrupt reads the edge
 *     time and moves the next sample point (resync), as a CAN controller does.
 *   - channel 1 interrupts at the sample point of every bit, half a bit after
 *     the last edge, and the interrupt reads PB8. One interrupt per bit.
 * can_rx.c turns the bits into frames (destuffing, CRC-15, form checks); the
 * interrupt puts each good frame in a buffer and loop() sends it to the PC.
 *
 * To the PC it looks like the receiver (usb_receiver), so
 * python/replay_dataset.py --receiver <this COM port> checks every frame:
 *   board -> PC   "READY STM32\n" at boot. The scripts' restart pulse (RTS)
 *                 never reaches this board on Windows, so they take the
 *                 once a second #STAT line as the sign it is alive
 *   board -> PC   frame record, 16 bytes: 0xA7 id_hi id_lo dlc d0..d7 t0 t1 t2 t3
 *                 t = microseconds (TIM2), stamped at the 6th end of frame bit,
 *                 where CAN counts the frame as received
 *   board -> PC   once a second: "#STAT rx=<frames sent to the PC> ovf=<frames
 *                 lost, buffer full> drop=0 peak=<most frames waiting>
 *                 buserr=<frames with errors> rec=0 stuff=<n> form=<n> crc=<n>
 *                 other=<extended or remote frames, not sent> isr=<longest bit
 *                 interrupt, CPU cycles at 168 MHz> lat=<latest sample after
 *                 its time, TIM4 ticks at 84 MHz>\n"
 *
 * Only standard data frames are sent to the PC, as the receiver does.
 */
#include <Arduino.h>
#include "can_rx.h"

#define FW_VERSION  4            // in READY and #STAT: which build is running
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
CAN_RX_FAST static inline void sample_at(uint16_t at)
{
    TIM4->CCR1 = at;
    if ((int16_t)(TIM4->CNT - at) >= 0)
        TIM4->EGR = TIM_EGR_CC1G;
}

CAN_RX_FAST static inline void sampling(bool on)
{
    if (on) {
        TIM4->SR = ~TIM_SR_CC1IF;
        TIM4->DIER |= TIM_DIER_CC1IE;
    } else {
        TIM4->DIER &= ~TIM_DIER_CC1IE;
    }
}

CAN_RX_FAST static void keep(const can_frame_t &f)
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

// The edge interrupt is only on while the bus is idle, to catch a start of
// frame. Inside a frame the timer still captures every edge in hardware, and
// the sample interrupt reads it: one interrupt per bit instead of two.
CAN_RX_FAST static inline void edge_irq(bool on)
{
    if (on) {
        TIM4->SR = ~(TIM_SR_CC3IF | TIM_SR_CC3OF);   // edges from the frame that ended
        TIM4->DIER |= TIM_DIER_CC3IE;
    } else {
        TIM4->DIER &= ~TIM_DIER_CC3IE;
    }
}

// Timing seen by the bit interrupt. One bit is 336 CPU cycles (168 MHz) and
// 168 TIM4 ticks (84 MHz). The sample point is half a bit after the edge, so a
// sample more than 84 ticks late reads the next bit; LATE_TICKS counts the
// ones that came close (over 3/4 of that margin).
#define LATE_TICKS  63
static volatile uint32_t isr_max = 0;    // longest interrupt, CPU cycles
static volatile uint16_t lat_max = 0;    // latest sample after its time, TIM4 ticks
static volatile uint32_t n_late = 0;     // samples more than LATE_TICKS late

extern "C" CAN_RX_FAST void TIM4_IRQHandler(void)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t sr = TIM4->SR;
    if ((sr & TIM_SR_CC1IF) && (TIM4->DIER & TIM_DIER_CC1IE)) {
        uint8_t level = (GPIOB->IDR >> 8) & 1;
        uint16_t due = TIM4->CCR1;
        uint16_t late = (uint16_t)(TIM4->CNT - due);
        TIM4->SR = ~TIM_SR_CC1IF;
        if (late > lat_max)
            lat_max = late;
        if (late > LATE_TICKS)
            n_late++;
        // A falling edge at the start of this bit moves the next sample point
        // (resync): half a bit after that edge, plus one bit.
        uint16_t next = due + bit_ticks;
        if (TIM4->SR & TIM_SR_CC3IF)
            next = TIM4->CCR3 + sample_ticks + bit_ticks;   // reading CCR3 clears the flag
        sample_at(next);
        can_frame_t f;
        if (can_rx_bit(level, &f))
            keep(f);
        if (can_rx_idle()) {
            sampling(false);
            edge_irq(true);
        }
    } else if ((sr & TIM_SR_CC3IF) && (TIM4->DIER & TIM_DIER_CC3IE)) {
        // Start of frame: hard sync. The edge itself says the bit is dominant,
        // so it is passed as 0 and the first sample is the next bit, 1.5 bits
        // after the edge. (Sampling it 0.5 bit after the edge lost frames whose
        // ID starts with a 1 when this first interrupt came late.)
        uint16_t at = TIM4->CCR3;          // reading it clears the flag
        edge_irq(false);
        can_frame_t f;
        can_rx_sof();
        can_rx_bit(0, &f);
        sampling(true);
        sample_at(at + sample_ticks + bit_ticks);
    }
    uint32_t took = DWT->CYCCNT - start;
    if (took > isr_max)
        isr_max = took;
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
    TIM4->DIER = 0;
    NVIC_SetPriority(TIM4_IRQn, 0);        // above USB (1) and SysTick (1, platformio.ini):
                                           // a late sample reads the wrong bit
    NVIC_EnableIRQ(TIM4_IRQn);
    TIM4->CR1 = TIM_CR1_CEN;

    // Cycle counter, to time the interrupt.
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Start by sampling until the bus has been idle for 11 bits (the edge
    // interrupt comes on once it is).
    can_rx_reset();
    sampling(true);
    sample_at(TIM4->CNT + bit_ticks);
}

// After an upload, dfu-util starts this program straight from the bootloader,
// which leaves the vector table pointing at the bootloader: USB then breaks
// ("A device attached to the system is not functioning") until a reset. Point
// it at this program before anything else runs, USB included.
__attribute__((constructor(101))) static void own_vector_table()
{
    SCB->VTOR = FLASH_BASE;
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
    Serial.printf("READY STM32 fw=%d\n", FW_VERSION);
}

// ---- restart into the USB bootloader without the BOOT0 jumper -------------------------

#define DFU_KEY     "DFU!"

// The STM32's built-in bootloader (system memory at 0x1FFF0000) does the USB
// DFU upload. Jumping there from this program saves moving the BOOT0 jumper.
// If it ever fails, the jumper still works.
static void enter_dfu()
{
    Serial.print("#DFU restarting into the USB bootloader\n");
    delay(50);                                   // let that line reach the PC

    NVIC_DisableIRQ(TIM4_IRQn);
    NVIC_DisableIRQ(OTG_FS_IRQn);
    __HAL_RCC_USB_OTG_FS_FORCE_RESET();
    __HAL_RCC_USB_OTG_FS_RELEASE_RESET();
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();

    // The board's D+ pull up is fixed, so the PC would not notice the USB
    // device restarting: hold D+ (PA12) low for a moment, an unplug.
    GPIO_InitTypeDef g = {};
    g.Pin = GPIO_PIN_12;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    GPIOA->BSRR = (uint32_t)GPIO_PIN_12 << 16;
    HAL_GPIO_Init(GPIOA, &g);
    delay(50);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);

    __disable_irq();
    HAL_RCC_DeInit();                            // back to the internal clock, as after reset
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    SCB->VTOR = 0;                               // address 0 is now the bootloader
    __enable_irq();

    uint32_t sp = *(volatile uint32_t *)0x1FFF0000;
    uint32_t pc = *(volatile uint32_t *)0x1FFF0004;
    __set_MSP(sp);
    ((void (*)(void))pc)();
    while (1) {
    }
}

// ---- loop: frames to the PC ----------------------------------------------------------

void loop()
{
    static bool rts_was = false;
    static uint32_t last_stat = 0, last_frame = 0;

    bool rts = Serial.rts();               // the scripts pulse RTS to restart a board; Windows
                                           // passes RTS on only along with a DTR change
    if (rts_was && !rts)
        Serial.printf("READY STM32 fw=%d\n", FW_VERSION);
    rts_was = rts;

    // "DFU!" from the PC (python/stm32_dfu.py): restart into the bootloader.
    static uint8_t matched = 0;
    while (Serial.available()) {
        char c = Serial.read();
        matched = c == DFU_KEY[matched] ? matched + 1 : (c == DFU_KEY[0]);
        if (matched == 4)
            enter_dfu();
    }

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
        Serial.printf("#STAT rx=%lu ovf=%lu drop=0 peak=%lu buserr=%lu rec=0 stuff=%lu form=%lu crc=%lu other=%lu"
                      " isr=%lu lat=%u nlate=%lu fw=%d\n",
                      (unsigned long)n_sent, (unsigned long)n_ovf, (unsigned long)peak,
                      (unsigned long)(s.stuff + s.form + s.crc), (unsigned long)s.stuff,
                      (unsigned long)s.form, (unsigned long)s.crc, (unsigned long)n_other,
                      (unsigned long)isr_max, (unsigned)lat_max, (unsigned long)n_late, FW_VERSION);
    }
}
