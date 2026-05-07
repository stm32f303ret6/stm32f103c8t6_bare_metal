/*
 * 05_tim_pwm — 1 kHz PWM on PA0 (TIM2_CH1) with software-ramped duty cycle.
 *
 * TIM2 is on APB1. After reset APB1 prescaler is /1, so
 *   TIM2_CLK = HCLK = 8 MHz (HSI default).
 *   (If APB1 prescaler != 1, the TIM clock is 2*PCLK1 — see RM0008 §7.2.)
 *
 * With PSC=7 the counter ticks at 8 MHz / 8 = 1 MHz, and ARR=999 makes the
 * period 1000 counts → 1 ms → 1 kHz PWM. The duty cycle is CCR1 / (ARR+1),
 * giving 0.1 % steps from 0 to 100 %.
 *
 * The compare register is double-buffered (preload via OC1PE) so a write to
 * CCR1 in the middle of a cycle does not glitch the output — the new value
 * is latched on the next update event.
 *
 * Pin: PA0 (TIM2_CH1) — alternate-function push-pull, 50 MHz drive. AFIO
 * remap is left at its reset default (no remap).
 */
#include "stm32f103xb.h"

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA0 = AF push-pull, 50 MHz. CRL nibble for pin 0 is bits 0..3. */
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOA->CRL |=  GPIO_CRL_CNF0_1 | GPIO_CRL_MODE0_0 | GPIO_CRL_MODE0_1;

    TIM2->PSC = 8U - 1U;          /* 8 MHz / 8 = 1 MHz counter tick      */
    TIM2->ARR = 1000U - 1U;       /* period = 1000 ticks → 1 kHz         */
    TIM2->CCR1 = 0U;              /* start at 0 % duty                   */

    /* CCMR1: channel 1 = output (CC1S=00), PWM mode 1 (OC1M=110),
     * preload enable (OC1PE=1) so CCR1 writes latch on the next UEV.  */
    TIM2->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;

    TIM2->CCER  = TIM_CCER_CC1E;  /* enable CH1 output                   */
    TIM2->CR1   = TIM_CR1_ARPE;   /* auto-reload preload (ARR shadowed)  */
    TIM2->EGR   = TIM_EGR_UG;     /* force update to load PSC/ARR        */
    TIM2->CR1  |= TIM_CR1_CEN;    /* go                                  */

    uint16_t duty = 0;
    int16_t  step = 10;
    while (1) {
        TIM2->CCR1 = duty;
        delay(20000);
        duty = (uint16_t)(duty + step);
        if (duty == 1000U)      step = -10;
        else if (duty == 0U)    step =  10;
    }
}
