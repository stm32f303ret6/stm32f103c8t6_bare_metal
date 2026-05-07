# 01 — GPIO blink

Blinks the on-board LED on the Blue Pill (PC13) using direct register access.

## Hardware

- **Pin:** PC13 (on-board LED, active-low)
- **Clock:** Default after reset (HSI, 8 MHz) — no clock tree configuration needed for this example.

## Register sequence

1. `RCC->APB2ENR |= RCC_APB2ENR_IOPCEN` — enable the GPIOC peripheral clock. Without this, writes to `GPIOC` registers are silently ignored.
2. `GPIOC->CRH` — clear `CNF13`/`MODE13`, then set `MODE13=10` to make PC13 a 2 MHz push-pull output (`CNF13=00`).
3. Toggle bit 13 of `GPIOC->ODR` in a `while(1)` with a busy-wait delay.

## Build & flash

```
make EXAMPLE=01_gpio_blink
make EXAMPLE=01_gpio_blink flash
```
