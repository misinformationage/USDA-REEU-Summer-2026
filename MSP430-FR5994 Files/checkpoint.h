#ifndef CHECKPOINT_H_
#define CHECKPOINT_H_
 
#include <stdint.h>

/* ============================================================
 * Power-loss SRAM checkpoint/resume module (MSP430FR5994)
 *
 * Periodically samples supply voltage on a dedicated ADC12 channel
 * (independent of any application ADC use) and, if it drops toward
 * an unsafe level, copies the live SRAM working region into FRAM
 * before power is lost. On next boot/reset, call checkpoint_init()
 * early -- if a valid checkpoint exists and voltage has recovered,
 * it restores SRAM and resumes execution at the point of interrupt
 * (this call does not return in that case).
 *
 * Voltage sensing uses the ADC12_B internal battery monitor (channel
 * 31, ADC12BATMAP) -- no external pin or divider required.
 *
 * VERIFY BEFORE USE:
 *  - REF_VOLTAGE_MV in checkpoint.c must match the exact voltage of
 *    whichever REFVSEL_x is selected, per the FR5994 family user's
 *    guide (SLAU367) -- this determines the accuracy of every
 *    threshold comparison.
 *  - CKPT_SRAM_BASE_ADDR/CKPT_META_BASE_ADDR/CKPT_FRAM_STACK must
 *    match whatever addresses your linker .cmd actually reserves for
 *    them (CKPT_SRAM_IMAGE/CKPT_META/CKPT_FRAM_STACK regions) -- keep
 *    the two files in sync if either changes.
 * ============================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* Call once, as early as possible in init (after basic clock setup,
 * before you rely on any global/static state). If this returns, no
 * resume happened and you're booting fresh (or continuing after a
 * shallow droop with no checkpoint pending). It also arms periodic
 * voltage monitoring for future checkpointing.
 */
void checkpoint_init(void);

/* Returns the most recently sampled supply voltage in millivolts.
 * Updated every ~205ms by the checkpoint ADC ISR (see checkpoint.c's
 * TA1 period). Safe to call from your main loop -- does a brief
 * interrupt-disable internally since it's a 32-bit read on a 16-bit
 * core and the ISR could preempt mid-read otherwise. Intended for
 * logging (e.g. via your own uart_print_string), not as a substitute
 * for the checkpoint policy itself.
 */
int32_t checkpoint_get_last_voltage_mv(void);

#ifdef __cplusplus
}
#endif

#endif /* CHECKPOINT_H_ */
