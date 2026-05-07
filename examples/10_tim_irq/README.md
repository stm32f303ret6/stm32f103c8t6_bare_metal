# 10 — TIM3 update interrupt

Blink PC13 at 2 Hz from a TIM3 update-event interrupt — the canonical
pattern for "do something on a hardware-timed schedule" using a
general-purpose peripheral timer (as opposed to the Cortex-M core's
SysTick used in `02_systick`).

## Hardware

- **Board:** Blue Pill (STM32F103C8T6).
- **LED:** PC13, active-low, on-board.

No external wiring needed.

## Clock plan

After reset the chip runs from HSI at 8 MHz and `SystemInit()` in `core/`
leaves it that way. APB1's prescaler is /1, so TIM3 (on APB1) is clocked
at `HCLK = 8 MHz`.

| Setting   | Value      | Effect                          |
|-----------|------------|---------------------------------|
| `PSC`     | 800 − 1    | Counter tick = 8 MHz / 800 = 10 kHz |
| `ARR`     | 5000 − 1   | Period = 5000 ticks = 500 ms (2 Hz) |
| `DIER`    | UIE        | Update event raises an interrupt    |

The visible blink rate is 1 Hz (toggle on each 500 ms event).

## Why TIM3?

- **TIM1** is the advanced-control timer — it has BRK/DTG/repetition logic
  the basic blink doesn't need, and its update IRQ shares a vector with
  TIM10 on some F1 variants. Avoid it for the simplest possible example.
- **TIM2** is already used by `05_tim_pwm`, so picking a different timer
  here lets the two examples be diffed without their register names
  clashing.
- **TIM3/TIM4** are plain general-purpose 16-bit timers; the choice
  between them is arbitrary.

## Two gotchas worth flagging

1. **`EGR = UG` sets UIF.** Forcing an update event to latch PSC/ARR also
   sets the update-interrupt flag. If you enable `TIM3_IRQn` in the NVIC
   without first clearing UIF, the handler fires immediately on the first
   `__WFI`. The example clears `SR` between `EGR = UG` and `DIER = UIE`.

2. **`SR.UIF` is rc_w0, not rc_w1.** To clear the flag from the ISR you
   write **0** to the bit (e.g. `TIM3->SR = ~TIM_SR_UIF`), not 1. Writing
   1 is a no-op, so the flag stays set, the NVIC re-pends the IRQ, and
   the handler re-enters forever. The same shape of bug shows up in
   `06_exti_button` (where `EXTI->PR` *is* w1c — opposite convention,
   different peripheral).

## Build & flash

```
make EXAMPLE=10_tim_irq
make EXAMPLE=10_tim_irq flash
```

PC13 should blink at ~1 Hz visible (2 Hz toggle).
