# 05 — TIM2 PWM

1 kHz PWM on PA0 (TIM2_CH1), duty cycle ramped 0 → 100 % → 0 in software.
Wire an LED + resistor between PA0 and GND to see it breathe.

## Hardware

- **Output:** PA0 (TIM2_CH1, default mapping — no AFIO remap)
- **Clock:** HSI 8 MHz post-reset (no PLL). APB1 prescaler /1 → TIM2 clock = 8 MHz

## Frequency math

```
TIM2_CLK = 8 MHz
PSC = 8 - 1     → counter at 8 MHz / 8 = 1 MHz (1 µs/tick)
ARR = 1000 - 1  → 1000 ticks/cycle  = 1 ms = 1 kHz
duty = CCR1 / (ARR + 1)           → 0.1 % resolution
```

> **Note on APB clocks:** when the APBx prescaler is /1, the timer kernel clock equals PCLK; when it's /2, /4, /8 or /16 the timer clock is *doubled* (= HCLK / prescaler × 2). On reset everything is /1, so timers run at HCLK.

## Register sequence

1. Enable IOPA on APB2 and TIM2 on APB1.
2. PA0 → AF push-pull 50 MHz (`CNF0=10`, `MODE0=11`).
3. Program PSC, ARR, CCR1 (initial 0 % duty).
4. `CCMR1`: `CC1S=00` (channel 1 = output), `OC1M=110` (PWM mode 1: high while CNT < CCR1), `OC1PE=1` (preload — writes to CCR1 latch on next update event, so mid-cycle changes don't glitch).
5. `CCER.CC1E=1` to drive the output pin.
6. `CR1.ARPE=1` so ARR is also double-buffered.
7. `EGR.UG=1` — software-generated update event copies PSC and ARR shadow values; without this the very first cycle uses uninitialised shadows.
8. `CR1.CEN=1` to start counting.

The main loop walks `CCR1` from 0 → 1000 → 0 in steps of 10, with a busy-wait between updates so the LED breathes visibly.

## PWM mode 1 vs 2

- **Mode 1 (110):** OC1 is high while `CNT < CCR1`, low afterwards. CCR1=0 → always low; CCR1>ARR → always high.
- **Mode 2 (111):** inverted polarity of the above.

`CCER.CC1P` flips the electrical polarity of the pin independently — useful for active-low loads.

## Build & flash

```
make EXAMPLE=05_tim_pwm
make EXAMPLE=05_tim_pwm flash
```
