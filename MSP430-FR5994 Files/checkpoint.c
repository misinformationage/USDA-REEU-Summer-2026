#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include "checkpoint.h"
 
/* ================= SRAM region to protect ================= */
#define SRAM_START_ADDR   (0x1C00u)
#define SRAM_SIZE_BYTES   (0x1000u)

/* ================= Internal supply-voltage monitor ================= */
#define CKPT_REFVSEL       REFVSEL_0
#define REF_VOLTAGE_MV     (1200L)

/* code = (AVCC/2) / Vref * 4095  =>  AVCC_mV = 2 * code * Vref_mV / 4095 */
#define ADC_CODE_TO_VCC_MV(code) \
    ((int32_t)(2L * (code) * REF_VOLTAGE_MV) / 4095L)

#define TH_19_MV  (1900)   /* deep droop, likely power-off imminent */
#define TH_22_MV  (2200)   /* checkpoint now */
#define TH_25_MV  (2500)   /* safe to resume */

/* ================= Fixed FRAM layout ================= */
#define CKPT_SRAM_BASE_ADDR   (0xEE00u)
#define CKPT_META_BASE_ADDR   (0xFF00u)

#define FLAG_CLEAR_VALUE  (0xFFFFu)
#define FLAG_SET_VALUE    (0x0000u)

static volatile uint16_t *ckpt_valid = (volatile uint16_t *)(uintptr_t)(CKPT_META_BASE_ADDR + 0u);
static volatile uint16_t *ckpt_sp    = (volatile uint16_t *)(uintptr_t)(CKPT_META_BASE_ADDR + 2u);
static volatile uint16_t *po_flag    = (volatile uint16_t *)(uintptr_t)(CKPT_META_BASE_ADDR + 4u);
static volatile uint16_t *ckpt_sram  = (volatile uint16_t *)(uintptr_t)(CKPT_SRAM_BASE_ADDR);

#pragma DATA_SECTION(ckpt_fram_stack, ".fram_stack")
volatile uint16_t ckpt_fram_stack[128];

volatile uint16_t ckpt_SP_ptr;
static volatile int32_t ckpt_last_voltage_mv = 3300; /* Initialize to sane default */

static inline uint8_t flag_is_set(volatile uint16_t *p)
{
    return (*p == FLAG_SET_VALUE) ? 1u : 0u;
}
static inline void flag_set(volatile uint16_t *p)   { *p = FLAG_SET_VALUE; }
static inline void flag_clear(volatile uint16_t *p) { *p = FLAG_CLEAR_VALUE; }

/* ================= Core policy ================= */
void checkpoint_poll(void)
{
    int32_t v_mv;
    uint16_t saved_ctl1;
    uint16_t timeout;

    /* --- SAFE ADC BORROWING LOGIC --- */
    /* 1. Disable ADC before changing configuration */
    ADC12CTL0 &= ~ADC12ENC;

    /* 2. Save main.cpp's full ADC control state */
    saved_ctl1 = ADC12CTL1;

    /* 3. Update CSTARTADD while preserving clock source & divider */
    ADC12CTL1 = (saved_ctl1 & ~(0x001F | ADC12SHP)) | ADC12SHP | ADC12CSTARTADD_1;

    /* 4. Clear stale flags and trigger conversion */
    ADC12IFGR0 &= ~ADC12IFG1;
    ADC12CTL0 |= ADC12ENC | ADC12SC;

    /* 5. Wait for conversion with safety timeout */
    timeout = 10000u;
    while (!(ADC12IFGR0 & ADC12IFG1) && --timeout);

    if (timeout > 0u) {
        v_mv = ADC_CODE_TO_VCC_MV(ADC12MEM1);
        ckpt_last_voltage_mv = v_mv;
    } else {
        v_mv = ckpt_last_voltage_mv; /* Fallback to last known good reading */
    }

    /* 6. Restore ADC configuration exactly as main.cpp configured it */
    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL1 = saved_ctl1;
    /* -------------------------------- */

    if (flag_is_set(ckpt_valid) && flag_is_set(po_flag)) {
        if (v_mv >= TH_25_MV) {
            uint16_t i;
            volatile uint16_t *sram = (volatile uint16_t *)(uintptr_t)SRAM_START_ADDR;
            uint16_t words = SRAM_SIZE_BYTES / 2u;

            for (i = 0; i < words; i++) {
                *(sram + i) = *(ckpt_sram + i);
            }
            ckpt_SP_ptr = *ckpt_sp;

            flag_clear(ckpt_valid);
            flag_clear(po_flag);
        }
        return;
    }

    if (flag_is_set(ckpt_valid) && !flag_is_set(po_flag)) {
        if (v_mv < TH_19_MV) {
            flag_set(po_flag);
            return;
        }
        if (v_mv >= TH_25_MV) {
            flag_clear(ckpt_valid);
        }
        return;
    }

    if (v_mv < TH_22_MV) {
        if (!flag_is_set(ckpt_valid)) {
            uint16_t i;
            volatile uint16_t *sram = (volatile uint16_t *)(uintptr_t)SRAM_START_ADDR;
            uint16_t words = SRAM_SIZE_BYTES / 2u;

            *ckpt_sp = ckpt_SP_ptr;
            for (i = 0; i < words; i++) {
                *(ckpt_sram + i) = *(sram + i);
            }
            flag_set(ckpt_valid);
        }
        if (v_mv < TH_19_MV) {
            flag_set(po_flag);
        }
        return;
    }
}

void checkpoint_init(void)
{
    /* Internal fixed voltage reference */
    while (REFCTL0 & REFGENBUSY);
    REFCTL0 = CKPT_REFVSEL | REFON;
    while (!(REFCTL0 & REFGENRDY));

    /* Route internal (AVCC)/2 onto channel 31 */
    ADC12CTL3 |= ADC12BATMAP;
    ADC12CTL0 |= ADC12ON;
    ADC12MCTL1 = ADC12INCH_31 | ADC12VRSEL_1 | ADC12EOS;

    /* Timer1_A periodically triggers the checkpoint poll */
    TA1CCR0  = 2048 - 1;
    TA1CCTL0 = CCIE;
    TA1CTL   = TASSEL__ACLK | MC__UP | TACLR;
}

int32_t checkpoint_get_last_voltage_mv(void)
{
    int32_t v;
    __disable_interrupt();
    v = ckpt_last_voltage_mv;
    __enable_interrupt();
    return v;
}

/* Standard C ISR: Runs cleanly on the normal SRAM stack during routine monitoring */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void CKPT_TA1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) CKPT_TA1_ISR(void)
#else
#error Compiler not supported!
#endif
{
    checkpoint_poll();
}
