/*
 * 06_exti_button — toggle PC13 (LED) on a falling edge of PA0 via EXTI0.
 *
 * Wire a momentary push-button between PA0 and GND. The internal pull-up
 * keeps the line high at rest; pressing pulls it low and triggers EXTI0.
 *
 * Each EXTI line on F1 is multiplexed across all GPIO ports through AFIO:
 * AFIO->EXTICR[0] field EXTI0 selects which port's pin 0 drives EXTI line 0.
 * That register lives in the AFIO block, so RCC_APB2ENR_AFIOEN must be on
 * before writing it.
 *
 * SysTick gives us a millisecond clock used to debounce — mechanical
 * switches bounce for several ms after a press, and a naive ISR would
 * toggle several times per press.
 */
#include "stm32f103xb.h"

#define HCLK_HZ        8000000U
#define DEBOUNCE_MS    50U

static volatile uint32_t ms_ticks;
static volatile uint32_t last_press_ms;

void SysTick_Handler(void) { ms_ticks++; }

void EXTI0_IRQHandler(void)
{
    /* W1C: write 1 to clear pending. Forget this and we re-enter forever. */
    EXTI->PR = EXTI_PR_PR0;

    uint32_t now = ms_ticks;
    if ((now - last_press_ms) < DEBOUNCE_MS) return;
    last_press_ms = now;

    GPIOC->ODR ^= GPIO_ODR_ODR13;
}

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
                 |  RCC_APB2ENR_IOPCEN
                 |  RCC_APB2ENR_AFIOEN;     /* required to write AFIO->EXTICR */

    /* PC13 = output 2 MHz push-pull (LED). */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* PA0 = input with pull-up: CNF0=10, MODE0=00, then ODR0=1 selects pull-up
     * (ODR0=0 would select pull-down on the same CNF/MODE encoding). */
    GPIOA->CRL  &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOA->CRL  |=  GPIO_CRL_CNF0_1;
    GPIOA->ODR  |=  GPIO_ODR_ODR0;

    /* EXTI line 0 source = port A. Reset value is 0 (= port A) but be explicit. */
    AFIO->EXTICR[0] &= ~AFIO_EXTICR1_EXTI0;
    AFIO->EXTICR[0] |=  AFIO_EXTICR1_EXTI0_PA;

    EXTI->IMR  |= EXTI_IMR_MR0;     /* unmask interrupt on line 0     */
    EXTI->FTSR |= EXTI_FTSR_TR0;    /* trigger on falling edge        */
    EXTI->RTSR &= ~EXTI_RTSR_TR0;   /* not on rising                  */

    NVIC_EnableIRQ(EXTI0_IRQn);

    SysTick->LOAD = (HCLK_HZ / 1000U) - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;

    while (1) { __asm__("wfi"); }   /* sleep until next interrupt */
}
