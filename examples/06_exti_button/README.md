# 06 — EXTI button

Toggle the on-board LED (PC13) on every press of a button wired between PA0
and GND. Demonstrates EXTI line configuration through AFIO, NVIC enable, and
a simple SysTick-based debounce.

## Hardware

- **Button:** momentary switch between PA0 and GND (no external pull — internal pull-up is used)
- **LED:** PC13 on-board, active-low
- **Clock:** HSI 8 MHz post-reset

## Why AFIO is involved

EXTI is a peripheral inside the core complex with 16 interrupt lines for
GPIOs (one line per pin number across all ports). Each line is multiplexed
across the ports through `AFIO->EXTICR[0..3]`:

```
EXTICR[0] selects the port for EXTI lines 0..3
EXTICR[1] selects the port for EXTI lines 4..7
EXTICR[2] selects the port for EXTI lines 8..11
EXTICR[3] selects the port for EXTI lines 12..15
```

So to use **PA0** as EXTI0, write `AFIO->EXTICR[0] EXTI0 = 0000` (port A).
The reset value is already 0 but writing it explicitly self-documents intent.

`RCC_APB2ENR_AFIOEN` must be enabled before any `AFIO` register write.

## Register sequence

1. Enable IOPA + IOPC + AFIO clocks on APB2.
2. PC13 → 2 MHz push-pull output (LED).
3. PA0 → input with pull-up (`CNF0=10`, `MODE0=00`, then `ODR0=1` selects pull-up; `ODR0=0` on the same encoding is pull-down).
4. Route EXTI line 0 to port A: `AFIO->EXTICR[0] EXTI0 = 0`.
5. `EXTI->IMR |= MR0`  — unmask line 0 to the NVIC.
6. `EXTI->FTSR |= TR0` — trigger on falling edge (button press pulls low).
7. `NVIC_EnableIRQ(EXTI0_IRQn)`.

## In the ISR

- `EXTI->PR = EXTI_PR_PR0` — pending bits are write-1-to-clear. **This must be the first thing in the handler**, or the NVIC will re-enter immediately on return.
- The debounce check `(now - last_press_ms) < 50` ignores edges that come within 50 ms of the previous accepted press, which covers the bounce window of a typical tactile switch.

## Lines 5-9 and 10-15

EXTI lines 5..9 share `EXTI9_5_IRQHandler` and lines 10..15 share `EXTI15_10_IRQHandler`. For those, the ISR has to look at `EXTI->PR` to decide which line(s) fired.

## Build & flash

```
make EXAMPLE=06_exti_button
make EXAMPLE=06_exti_button flash
```
