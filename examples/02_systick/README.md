# 02 — SysTick

Drives a 1 kHz tick from the Cortex-M3 SysTick timer and blinks PC13 every 500 ms.

## Hardware

- **LED:** PC13 (on-board, active-low)
- **Clock:** HSI 8 MHz (post-reset default — no PLL/HSE setup)

## Register sequence

1. `RCC->APB2ENR |= IOPCEN`, configure PC13 as a 2 MHz push-pull output (same as `01_gpio_blink`).
2. Program SysTick (CMSIS `SysTick` struct, defined in `core_cm3.h`):
   - `LOAD = HCLK / 1000 - 1` — 24-bit reload value, fires every 1 ms.
   - `VAL  = 0` — clears the current counter and the COUNTFLAG.
   - `CTRL = CLKSOURCE | TICKINT | ENABLE`:
     - `CLKSOURCE = 1` selects the processor clock (HCLK). With `0` it would use HCLK/8.
     - `TICKINT = 1` raises the SysTick exception on each underflow.
     - `ENABLE = 1` starts the counter.
3. `SysTick_Handler` increments a 32-bit `ms_ticks` counter. Reads of a 32-bit aligned word are atomic on Cortex-M3, so `main()` can sample it without disabling interrupts.

The wraparound-safe schedule check `(int32_t)(now - next) >= 0` keeps working correctly even after `ms_ticks` rolls past 2^32 (~49 days).

## Build & flash

```
make EXAMPLE=02_systick
make EXAMPLE=02_systick flash
```
