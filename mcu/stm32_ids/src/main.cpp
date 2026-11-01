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
 *                 its time, TIM4 ticks> nlate=<samples over 94 ticks late>
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

#define FW_VERSION  12          // in READY and #STAT: which build is running
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

// ---- M3: prevention (error flag on PB9) -----------------------------------------------
//
// Off at power up; 'I' over USB turns it on, 'i' off. When the model calls the
// frame on the bus an attack, the bit interrupt starts an active error flag at
// the next bit boundary: PB9 (the module's TX) dominant for 6 bit times. That
// breaks the stuffing rule, so every node throws the frame away, and the
// sender's transmit error counter rises by 8; an attacker that keeps sending
// goes error passive and then bus off.
//
// PB9 is TIM4 channel 4, the timer that does the bit timing, so the flag
// starts and ends on the exact TIM4 tick in hardware (output compare "active
// on match", then "inactive on match"), not when an interrupt gets round to it.
// Pin: open drain, inactive = released (the TJA1050 pulls TX up = recessive),
// active = low = dominant.

#define FLAG_BITS       6
#define OC_ACTIVE       1                // OC4M: set active on match
#define OC_INACTIVE     2                // OC4M: set inactive on match
#define OC_OFF          4                // OC4M: force inactive (released)
#define MARKER_ID       0x7FF            // the replay's start marker, never blocked

static volatile bool ips_on = false;
static volatile uint8_t m3_pending = 0;  // the bit interrupt's only M3 test: a flag is wanted,
                                         // scheduled or on the bus
static volatile uint32_t allow_seq = 0;  // frame the model called an attack but the allow list kept
static uint8_t flag_state = 0;           // 0 none, 1 scheduled or on the bus, 2 end scheduled
static uint16_t flag_start, flag_end;    // TIM4 ticks
static volatile uint32_t n_rej = 0;      // error flags sent
static volatile uint32_t n_ilate = 0;    // attack verdicts that came after the end of frame field began
static volatile uint32_t n_allow = 0;    // attack verdicts on allowed frames
static volatile uint32_t n_abort = 0;    // flags cut short by the safety limit (should stay 0)

static inline void set_oc4(uint32_t mode)
{
    TIM4->CCMR2 = (TIM4->CCMR2 & ~TIM_CCMR2_OC4M) | (mode << TIM_CCMR2_OC4M_Pos);
}

// The allow list: frames the IPS must never destroy, whatever the model says.
static bool allowed(uint16_t id, const uint8_t *d)
{
    return id == MARKER_ID && d[0] == 'C' && d[1] == 'A' && d[2] == 'N' && d[3] == 'S' && d[4] == 'T';
}

static void ips_enable(bool on)
{
    GPIO_InitTypeDef g = {};
    g.Pin = GPIO_PIN_9;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    set_oc4(OC_OFF);                     // released before the pin is handed over
    flag_state = 0;
    m3_pending = 0;
    if (on) {
        TIM4->CCER |= TIM_CCER_CC4P | TIM_CCER_CC4E;    // active level = low
        g.Mode = GPIO_MODE_AF_OD;
        g.Alternate = GPIO_AF2_TIM4;
    } else {
        GPIOB->BSRR = GPIO_PIN_9;        // high first: released, as at power up
        g.Mode = GPIO_MODE_OUTPUT_OD;
    }
    HAL_GPIO_Init(GPIOB, &g);
    if (!on)
        TIM4->CCER &= ~TIM_CCER_CC4E;
    ips_on = on;
}

// Bit interrupt, at the sample point `due`: start the flag at the next bit
// boundary (bit_ticks - sample_ticks after the sample point).
static void flag_begin(uint16_t due)
{
    if (!can_rx_before_eof())            // too late: the frame stands, an alert only
        return;                          // (keep() counts it in n_ilate when it completes)
    uint16_t at = due + (bit_ticks - sample_ticks);
    if ((int16_t)(at - TIM4->CNT) < 8)   // boundary (nearly) passed: take the next one,
        at += bit_ticks;                 // a missed compare would fire 780 us later
    flag_start = at;
    flag_end = at + FLAG_BITS * bit_ticks;
    TIM4->CCR4 = at;
    set_oc4(OC_ACTIVE);
    flag_state = 1;
    can_rx_quiet = 1;
    n_rej++;
}

// Bit interrupt, every bit while a flag is pending: once it is on the bus,
// schedule its end; once it has ended, release the pin for good.
static void flag_step(void)
{
    uint16_t now = TIM4->CNT;
    int16_t since = (int16_t)(now - flag_start);
    if (flag_state == 1 && since >= 0) {
        TIM4->CCR4 = flag_end;
        set_oc4(OC_INACTIVE);
        flag_state = 2;
    } else if (flag_state == 2 && (int16_t)(now - flag_end) >= 0) {
        set_oc4(OC_OFF);
        flag_state = 0;
    }
    if (flag_state && since > (int16_t)((FLAG_BITS + 2) * bit_ticks)) {   // never hold the bus
        set_oc4(OC_OFF);
        flag_state = 0;
        n_abort++;
    }
}

extern "C" void TIM7_IRQHandler(void)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t seq = job_seq;
    int cls = classify(job.id, job.dlc, job.d);
    res_cls = (uint8_t)cls;
    res_tick = (uint16_t)TIM4->CNT;
    __DMB();
    res_seq = seq;
    if (ips_on && cls != 0) {
        if (allowed(job.id, job.d)) {
            allow_seq = seq;
            n_allow++;
        } else {
            __disable_irq();             // the frame on the bus is still this one:
            if (seq == frame_seq)        // the bit interrupt starts the flag
                m3_pending = 1;
            __enable_irq();
        }
    }
    uint32_t took = DWT->CYCCNT - start;
    if (took > cls_max)
        cls_max = took;
}

// ---- bit timing (TIM4 interrupt) -----------------------------------------------------

// Next sample at TIM4 tick `at`. If that moment has already passed (a late
// interrupt), sample at once instead of waiting for the 16 bit counter to wrap.
// Bit interrupt, only while m3_pending: start the flag, step it, and clear
// m3_pending once it is over. Kept out of the bit interrupt (noinline) so the
// other ~120 bits of a frame pay one byte test for M3, nothing more.
static __attribute__((noinline)) void m3_step(uint16_t due)
{
    if (flag_state)
        flag_step();
    else
        flag_begin(due);
    m3_pending = flag_state != 0;
}

// TIM4 is on APB1: every register read costs the bit interrupt several CPU
// cycles, so what it can know from RAM it keeps in RAM (fw 12): the sample
// time it set last and which of its two interrupts is on.
static uint16_t due_tick;                // the sample time last written to CCR1
static uint8_t sampling_on, edge_on;     // CC1IE and CC3IE as last set

// Next sample at TIM4 tick `at`; `now` is a recent TIM4->CNT. If that moment
// has passed, or is closer than SAMPLE_SLACK ticks (the time from reading
// `now` to here), sample at once instead of waiting for the 16 bit counter to
// wrap. At once is at most SAMPLE_SLACK ticks early: still after the edge.
#define SAMPLE_SLACK 24
static inline void sample_at(uint16_t at, uint16_t now)
{
    TIM4->CCR1 = at;
    due_tick = at;
    if ((int16_t)(now + SAMPLE_SLACK - at) >= 0)
        TIM4->EGR = TIM_EGR_CC1G;
}

static inline void sampling(bool on)
{
    sampling_on = on;
    if (on) {
        TIM4->SR = ~TIM_SR_CC1IF;
        TIM4->DIER |= TIM_DIER_CC1IE;
    } else {
        TIM4->DIER &= ~TIM_DIER_CC1IE;
    }
}

// Features complete: hand them to the model interrupt. Kept short: a copy
// and a pending flag, the model itself runs at a lower priority.
// noinline (here and keep()): fw 5 inlined both into the bit interrupt, which
// then saved 9 registers on every bit instead of fw 3's 5. They run once per
// frame, so the call costs nothing that matters; the bigger entry cost on all
// ~120 bits of every frame did.
static __attribute__((noinline)) void features_in(const can_frame_t &f, uint16_t due)
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

static __attribute__((noinline)) void keep(const can_frame_t &f, uint16_t end_tick)
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
        if (ips_on && e.cls != 0 && frame_seq != allow_seq)
            n_ilate++;                   // an attack verdict, but the frame got through
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
    edge_on = on;
    if (on) {
        TIM4->SR = ~(TIM_SR_CC3IF | TIM_SR_CC3OF);   // edges from the frame that ended
        TIM4->DIER |= TIM_DIER_CC3IE;
    } else {
        TIM4->DIER &= ~TIM_DIER_CC3IE;
    }
}

// Timing seen by the bit interrupt. One bit is 336 CPU cycles (168 MHz) and
// 168 TIM4 ticks (84 MHz). The sample point is a quarter bit after the edge, so
// a sample more than 126 ticks late reads the next bit; LATE_TICKS counts the
// ones that came close (over 3/4 of that margin).
#define LATE_TICKS  94
static volatile uint32_t isr_max = 0;    // longest interrupt, CPU cycles
static volatile uint16_t lat_max = 0;    // latest sample after its time, TIM4 ticks
static volatile uint32_t n_late = 0;     // samples more than LATE_TICKS late

// Diagnostic switches (fw 7), set over USB, see loop().
static volatile bool feat_on = true;     // 'F'/'f': hand the features to the model mid frame
static volatile bool prio_fw3 = false;   // 'p'/'P': fw 3's priorities (USB 1, SysTick 1, no model IRQ)

extern "C" void TIM4_IRQHandler(void)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t sr = TIM4->SR;
    if ((sr & TIM_SR_CC1IF) && sampling_on) {
        uint8_t level = (GPIOB->IDR >> 8) & 1;
        uint16_t due = due_tick;
        uint16_t now = TIM4->CNT;
        int16_t late = (int16_t)(now - due);
        TIM4->SR = ~TIM_SR_CC1IF;
        if (late > (int16_t)lat_max)
            lat_max = late;
        if (late > LATE_TICKS)
            n_late++;
        // A falling edge at the start of this bit moves the next sample point
        // (resync): sample_ticks after that edge, plus one bit. The edge came
        // before this sample point, so the SR read on entry already shows it.
        uint16_t next = due + bit_ticks;
        if (sr & TIM_SR_CC3IF)
            next = TIM4->CCR3 + sample_ticks + bit_ticks;   // reading CCR3 clears the flag
        sample_at(next, now);
        if (m3_pending)                  // M3: an error flag is wanted or on the bus
            m3_step(due);
        can_frame_t f;
        int got = can_rx_bit(level, &f);
        if (got == CAN_RX_FEATURES)
            features_in(f, due);
        else if (got == CAN_RX_FRAME)
            keep(f, due);
        else if (got == CAN_RX_IDLE) {
            can_rx_quiet = 0;            // our flag (if any) and its error frame are over
            sampling(false);
            edge_irq(true);
        }
    } else if ((sr & TIM_SR_CC3IF) && edge_on) {
        // Start of frame: hard sync. The edge itself says the bit is dominant,
        // so it is passed as 0 and the first sample is the next bit, one bit
        // plus sample_ticks after the edge. (Sampling it 0.5 bit after the edge lost frames whose
        // ID starts with a 1 when this first interrupt came late.)
        uint16_t at = TIM4->CCR3;          // reading it clears the flag
        edge_irq(false);
        sof_tick = at;
        frame_seq = 0;
        can_frame_t f;
        can_rx_sof();
        can_rx_bit(0, &f);
        sampling(true);
        sample_at(at + sample_ticks + bit_ticks, TIM4->CNT);
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
    // Sample a quarter bit after the edge: a late interrupt then has 3/4 of a
    // bit before it reads the next bit (half a bit left fw 8 with no margin).
    // Every falling edge resyncs, so the quarter is measured from a real edge.
    sample_ticks = bit_ticks / 4;

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
    sample_at(TIM4->CNT + bit_ticks, TIM4->CNT);
}

// fw 3 had USB and SysTick at 1 and no model interrupt; fw 5 moved both to 2
// and enabled the model's (TIM7) vector. The bit interrupt stays at 0 either way.
static void set_priorities(bool like_fw3)
{
    prio_fw3 = like_fw3;
    NVIC_SetPriority(OTG_FS_IRQn, like_fw3 ? 1 : 2);
    NVIC_SetPriority(SysTick_IRQn, like_fw3 ? 1 : 2);
    if (like_fw3)
        NVIC_DisableIRQ(MODEL_IRQ);
    else
        NVIC_EnableIRQ(MODEL_IRQ);
}

static void zero_counters()
{
    __disable_irq();
    isr_max = 0;
    lat_max = 0; n_late = 0;
    cls_max = 0; n_dlate = 0;
    n_rej = 0; n_ilate = 0; n_allow = 0; n_abort = 0;
    n_ovf = 0; n_other = 0; peak = 0;
    can_rx_stats.stuff = can_rx_stats.form = can_rx_stats.crc = 0;
    __enable_irq();
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

    // 'C'/'c' model on/off. Diagnostics (fw 7): 'F'/'f' features step on/off
    // (off: no mid frame copy at all), 'P'/'p' fw 5 / fw 3 priorities,
    // 'Z' zero the timing and error counters (rx keeps counting).
    while (Serial.available()) {
        int c = Serial.read();
        if (c == 'C')
            model_on = true;
        else if (c == 'c')
            model_on = false;
        else if (c == 'F' || c == 'f')
            feat_on = can_rx_features_on = (c == 'F');
        else if (c == 'P' || c == 'p')
            set_priorities(c == 'p');
        else if (c == 'Z')
            zero_counters();
        else if (c == 'I' || c == 'i')   // M3: prevention on / off
            ips_enable(c == 'I');
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
                      " isr=%lu lat=%u nlate=%lu model=%s cmax=%lu dlate=%lu"
                      " feat=%s prio=%s ips=%s rej=%lu ilate=%lu allow=%lu abort=%lu fw=%d id=%s\n",
                      (unsigned long)n_sent, (unsigned long)n_ovf, (unsigned long)peak,
                      (unsigned long)(s.stuff + s.form + s.crc), (unsigned long)s.stuff,
                      (unsigned long)s.form, (unsigned long)s.crc, (unsigned long)n_other,
                      (unsigned long)isr_max, (unsigned)lat_max, (unsigned long)n_late,
                      model_on ? "on" : "off", (unsigned long)cls_max, (unsigned long)n_dlate,
                      feat_on ? "on" : "off", prio_fw3 ? "fw3" : "fw5",
                      ips_on ? "on" : "off", (unsigned long)n_rej, (unsigned long)n_ilate,
                      (unsigned long)n_allow, (unsigned long)n_abort,
                      FW_VERSION, classify_model_id);
    }
}
