/*
 * 01_gpio_blink — Blink the on-board LED on the Blue Pill (PC13).
 *
 * Pure register-level access: no HAL, no SysTick, no interrupts.
 * The LED on the Blue Pill is wired active-low between 3V3 and PC13,
 * so writing 0 turns it ON and writing 1 turns it OFF.
 *
 * Steps:
 *   1. Enable the GPIOC peripheral clock in RCC->APB2ENR.
 *   2. Configure PC13 as a 2 MHz push-pull output by writing CNF13=00,
 *      MODE13=10 into the high control register GPIOC->CRH.
 *   3. Toggle PC13 via GPIOC->ODR with a busy-wait delay.
 */
#include "stm32f103xb.h"

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    while (1) {
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        delay(200000);
    }
}
