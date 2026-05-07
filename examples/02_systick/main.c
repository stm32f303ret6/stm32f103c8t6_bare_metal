/*
 * 02_systick — 1 ms tick using the Cortex-M3 SysTick timer.
 *
 * SysTick is a 24-bit down counter inside the Cortex-M3 core (not a peripheral
 * on any APB bus), so it is always available and is the canonical "OS tick"
 * source on Cortex-M parts. CMSIS exposes it through the `SysTick` struct in
 * core_cm3.h with fields CTRL / LOAD / VAL / CALIB.
 *
 * After reset the chip runs from HSI at 8 MHz, and SystemInit() in core/ does
 * not change that. We feed SysTick from the processor clock (HCLK = 8 MHz),
 * so a reload value of 8000 - 1 makes the counter wrap once every 1 ms.
 *
 * The handler increments a software ms counter; main() polls it and toggles
 * PC13 (active-low LED) once per 500 ms.
 */
#include "stm32f103xb.h"

#define HCLK_HZ      8000000U
#define TICK_HZ      1000U
#define BLINK_PERIOD 500U

static volatile uint32_t ms_ticks;

void SysTick_Handler(void) { ms_ticks++; }

static uint32_t millis(void)
{
    /* 32-bit aligned read is atomic on Cortex-M3, no critical section needed. */
    return ms_ticks;
}

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    SysTick->LOAD = (HCLK_HZ / TICK_HZ) - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;

    uint32_t next = millis() + BLINK_PERIOD;
    while (1) {
        if ((int32_t)(millis() - next) >= 0) {
            GPIOC->ODR ^= GPIO_ODR_ODR13;
            next += BLINK_PERIOD;
        }
    }
}
