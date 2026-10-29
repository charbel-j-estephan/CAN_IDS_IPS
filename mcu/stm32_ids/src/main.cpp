/* STM32 IDS node. M1: receives CAN frames in software, bit by bit.
 * M2: classifies every frame with the random forest while it is on the bus.
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
 * can_rx.c turns the bits into frames (destuffing, CRC-15, form checks).
 *
 * M2: once the ID, DLC and first 5 data bytes are in, the bit interrupt hands
 * them to a lower priority interrupt (TIM7's vector, triggered in software)
 * that runs the model (classify.c), while the rest of the frame is still
 * arriving. Priorities: bit timing 0, model 1, USB and SysTick 2, so a
 * decision never waits for USB. The good frame, its class and when the
 * decision came go to loop(), which sends them to the PC.
 *
 * Serial protocol (USB):
 *   board -> PC   "READY STM32 fw=<n> model=<id>\n" at boot. The scripts'
 *                 restart pulse (RTS) never reaches this board on Windows, so
 *                 they take the once a second #STAT line as the sign it is alive
 *   board -> PC   frame record, 23 bytes:
 *                   0xA8 id_hi id_lo dlc d0..d7 t0..t3 cls r0 r1 c0 c1 e0 e1
 *                 t = microseconds (TIM2) at the 6th end of frame bit, where CAN
 *                 counts the frame as received. cls = class (0 Normal, 1 DoS,
 *                 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof), 0xFF no decision by the
 *                 end of the frame, 0xFE model switched off. r, c, e = TIM4
 *                 ticks (84 MHz, little endian) from the start of frame edge to:
 *                 r the 5th data byte's last bit (features complete), c the
 *                 decision, e the 6th end of frame bit
 *   PC -> board   'C' model on (default), 'c' model off
 *   board -> PC   once a second: "#STAT rx=<frames sent to the PC> ovf=<frames
 *                 lost, buffer full> drop=0 peak=<most frames waiting>
 *                 buserr=<frames with errors> rec=0 stuff=<n> form=<n> crc=<n>
 *                 other=<extended or remote frames, not sent> isr=<longest bit
 *                 interrupt, CPU cycles at 168 MHz> lat=<latest sample after
 *                 its time, TIM4 ticks> nlate=<samples over 63 ticks late>
 *                 model=<on|off> cmax=<longest model run, CPU cycles>
 *                 dlate=<frames without a decision by their end> fw=<n>
 *                 id=<model id>\n"
 *
 * Only standard data frames are sent to the PC and classified, as the
 * receiver does.
 */
#include <Arduino.h>
#include "can_rx.h"
#include "classify.h"

#define FW_VERSION  6            // in READY and #STAT: which build is running
#define BITRATE     500000
#define RING        512          // frames waiting for USB, power of two
#define LED         PA6          // on (low) while frames arrive
#define REC_LEN     23
#define CLS_LATE    0xFF
#define CLS_OFF     0xFE
#define MODEL_IRQ   TIM7_IRQn    // free vector (TIM7 itself is not used), set pending in software

struct Frame {
    uint32_t ts;
    can_frame_t f;
    uint8_t cls;
    uint16_t ready, decided, end;        // TIM4 ticks after the start of frame edge
};

static Frame ring[RING];
static volatile uint32_t ring_head = 0, ring_tail = 0;   // the interrupt moves head, loop() tail
static volatile uint32_t n_ovf = 0, n_other = 0;
static uint32_t n_sent = 0, peak = 0;

static uint16_t bit_ticks, sample_ticks;                 // TIM4 ticks

// ---- M2: the model runs in its own interrupt ------------------------------------------

static volatile bool model_on = true;

// The frame on the bus now (bit interrupt only).
static uint16_t sof_tick, ready_tick;
static uint32_t frame_seq = 0;           // this frame's job number, 0 until its features went to the model
static uint32_t seq_counter = 0;         // job numbers keep counting across frames, never 0

// Bit interrupt -> model interrupt.
static struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t d[CAN_RX_FEATURE_BYTES];
} job;
static volatile uint32_t job_seq = 0;

// Model interrupt -> bit interrupt. res_seq is written last.
static volatile uint8_t res_cls;
static volatile uint16_t res_tick;
static volatile uint32_t res_seq = 0;

static volatile uint32_t cls_max = 0;    // longest model run, CPU cycles
static volatile uint32_t n_dlate = 0;    // frames whose decision had not come by their end

extern "C" void TIM7_IRQHandler(void)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t seq = job_seq;
    int cls = classify(job.id, job.dlc, job.d);
    res_cls = (uint8_t)cls;
    res_tick = (uint16_t)TIM4->CNT;
    __DMB();
    res_seq = seq;
    uint32_t took = DWT->CYCCNT - start;
    if (took > cls_max)
        cls_max = took;
}

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

// Features complete: hand them to the model interrupt. Kept short: a copy
// and a pending flag, the model itself runs at a lower priority.
static inline void features_in(const can_frame_t &f, uint16_t due)
{
    ready_tick = due;
    if (!model_on || f.ide || f.rtr)
        return;
    job.id = f.id;
    job.dlc = f.dlc;
    for (int i = 0; i < CAN_RX_FEATURE_BYTES; i++)
        job.d[i] = f.data[i];
    if (++seq_counter == 0)
        seq_counter = 1;
    frame_seq = seq_counter;
    job_seq = frame_seq;
    NVIC_SetPendingIRQ(MODEL_IRQ);
}

static void keep(const can_frame_t &f, uint16_t end_tick)
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
    Frame &e = ring[ring_head];
    e.ts = TIM2->CNT;
    e.f = f;
    e.ready = ready_tick - sof_tick;
    e.end = end_tick - sof_tick;
    if (!model_on) {
        e.cls = CLS_OFF;
        e.decided = 0;
    } else if (frame_seq && res_seq == frame_seq) {
        e.cls = res_cls;
        e.decided = res_tick - sof_tick;
    } else {
        e.cls = CLS_LATE;
        e.decided = 0;
        n_dlate++;
    }
    ring_head = next;
}

// The edge interrupt is only on while the bus is idle, to catch a start of
// frame. Inside a frame the timer still captures every edge in hardware, and
// the sample interrupt reads it: one interrupt per bit instead of two.
static inline void edge_irq(bool on)
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

extern "C" void TIM4_IRQHandler(void)
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
        int got = can_rx_bit(level, &f);
        if (got == CAN_RX_FEATURES)
            features_in(f, due);
        else if (got == CAN_RX_FRAME)
            keep(f, due);
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
        sof_tick = at;
        frame_seq = 0;
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

    // Priorities: bit timing above everything (a late sample reads the wrong
    // bit), the model next (a decision must not wait for USB), then USB and
    // SysTick (2, set in setup() and platformio.ini).
    NVIC_SetPriority(TIM4_IRQn, 0);
    NVIC_SetPriority(MODEL_IRQ, 1);
    NVIC_EnableIRQ(MODEL_IRQ);
    NVIC_EnableIRQ(TIM4_IRQn);
    TIM4->CR1 = TIM_CR1_CEN;

    // Cycle counter, to time the interrupts.
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Start by sampling until the bus has been idle for 11 bits (the edge
    // interrupt comes on once it is).
    can_rx_reset();
    sampling(true);
    sample_at(TIM4->CNT + bit_ticks);
}

static void ready_line()
{
    Serial.printf("READY STM32 fw=%d model=%s\n", FW_VERSION, classify_model_id);
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
    NVIC_SetPriority(OTG_FS_IRQn, 2);      // below the model, see start_timers()
    start_timers();
    ready_line();
}

// ---- loop: frames to the PC ----------------------------------------------------------

void loop()
{
    static bool rts_was = false;
    static uint32_t last_stat = 0, last_frame = 0;

    bool rts = Serial.rts();               // the scripts pulse RTS to restart a board; Windows
                                           // passes RTS on only along with a DTR change
    if (rts_was && !rts)
        ready_line();
    rts_was = rts;

    while (Serial.available()) {           // 'C' model on, 'c' model off
        int c = Serial.read();
        if (c == 'C')
            model_on = true;
        else if (c == 'c')
            model_on = false;
    }

    uint32_t waiting = (ring_head - ring_tail) & (RING - 1);
    if (waiting > peak)
        peak = waiting;
    while (ring_tail != ring_head && Serial.availableForWrite() >= REC_LEN) {
        const Frame &e = ring[ring_tail];
        uint8_t r[REC_LEN];
        r[0] = 0xA8;
        r[1] = (e.f.id >> 8) & 0x07;
        r[2] = e.f.id & 0xFF;
        r[3] = e.f.dlc > 8 ? 8 : e.f.dlc;
        memcpy(&r[4], e.f.data, 8);
        r[12] = e.ts;
        r[13] = e.ts >> 8;
        r[14] = e.ts >> 16;
        r[15] = e.ts >> 24;
        r[16] = e.cls;
        r[17] = e.ready;
        r[18] = e.ready >> 8;
        r[19] = e.decided;
        r[20] = e.decided >> 8;
        r[21] = e.end;
        r[22] = e.end >> 8;
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
                      " isr=%lu lat=%u nlate=%lu model=%s cmax=%lu dlate=%lu fw=%d id=%s\n",
                      (unsigned long)n_sent, (unsigned long)n_ovf, (unsigned long)peak,
                      (unsigned long)(s.stuff + s.form + s.crc), (unsigned long)s.stuff,
                      (unsigned long)s.form, (unsigned long)s.crc, (unsigned long)n_other,
                      (unsigned long)isr_max, (unsigned)lat_max, (unsigned long)n_late,
                      model_on ? "on" : "off", (unsigned long)cls_max, (unsigned long)n_dlate,
                      FW_VERSION, classify_model_id);
    }
}
