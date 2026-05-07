/*
 * 10_tim_irq — TIM3 update-event interrupt toggling PC13 (LED) at 2 Hz.
 *
 * Unlike 02_systick (which uses the Cortex-M core's SysTick) this example
 * uses a general-purpose peripheral timer and routes its interrupt through
 * the NVIC. The pattern generalises to TIM2/4 as well — only the IRQ
 * number, the RCC enable bit, and the APB bus differ.
 *
 * TIM3 lives on APB1. After reset APB1's prescaler is /1, so
 *   TIM3_CLK = HCLK = 8 MHz (HSI default).
 *   (If APB1 prescaler != 1 the timer clock becomes 2*PCLK1 — see RM0008
 *   §7.2 — but here it doesn't apply.)
 *
 * With PSC=800-1 the counter ticks at 8 MHz / 800 = 10 kHz. ARR=5000-1
 * makes the period 5000 ticks → 500 ms, so the update event fires at 2 Hz
 * and the LED toggles at 2 Hz (1 Hz visible blink).
 *
 * Two write-order details that bite the unwary:
 *
 *  1. EGR=UG forces an update event to load PSC/ARR from their preload
 *     registers. That update *also* sets UIF, so we must clear it before
 *     enabling the interrupt or the ISR will fire immediately on entry.
 *
 *  2. UIF is rc_w0: the handler must write 0 (NOT 1) to the bit to clear
 *     it. Forgetting this re-enters the ISR forever — the same shape of
 *     bug as forgetting EXTI->PR in 06_exti_button.
 */
#include "stm32f103xb.h"

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR = ~TIM_SR_UIF;     /* rc_w0: write 0 to clear */
        GPIOC->ODR ^= GPIO_ODR_ODR13;
    }
}

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* PC13 = output 2 MHz push-pull (active-low LED on the Blue Pill). */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    TIM3->PSC = 800U  - 1U;         /* 8 MHz / 800 = 10 kHz tick   */
    TIM3->ARR = 5000U - 1U;         /* 5000 ticks → 500 ms period  */
    TIM3->CR1 = TIM_CR1_ARPE;       /* ARR preload                 */
    TIM3->EGR = TIM_EGR_UG;         /* latch PSC/ARR (sets UIF)    */
    TIM3->SR  = ~TIM_SR_UIF;        /* clear the spurious UIF      */
    TIM3->DIER = TIM_DIER_UIE;      /* enable update interrupt     */

    NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1 |= TIM_CR1_CEN;

    while (1) { __asm__("wfi"); }   /* sleep until next interrupt */
}
