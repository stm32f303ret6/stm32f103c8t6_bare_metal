# 07 — ADC1 single conversion

Reads PA0 (ADC1 channel 0) and prints raw + millivolt values over USART1
once per second.

## Hardware

- **Input:**  PA0 — wire a 10 kΩ pot between 3V3 and GND, wiper to PA0.
- **TX:**     PA9 (USART1_TX) → USB-serial dongle, 9600 8-N-1
- **VREF+:**  Tied to 3V3 on the Blue Pill (no separate analog reference)
- **Clock:**  HSI 8 MHz post-reset

## Clocking the ADC

ADC clock comes from PCLK2 through the dedicated `RCC_CFGR.ADCPRE` divider
and **must not exceed 14 MHz**. Reset value is `/2`, giving 4 MHz here. If
you ever switch to a 72 MHz system clock (PCLK2 = 72 MHz / 1) you must use
`/6` (= 12 MHz) — `/4` (= 18 MHz) violates the spec.

## Calibration

Per RM0008 §11.4 the calibration sequence is:

1. `ADON = 1` to power on, wait ≥ 1 µs (tSTAB) for the analog block to settle.
2. `RSTCAL = 1`, wait until it self-clears.
3. `CAL    = 1`, wait until it self-clears.

Skipping calibration costs a few LSBs of offset and worsens the linearity
spec. It only needs to be run once after power-on (or after Vdd changes
significantly).

## Conversion

For a single regular conversion on one channel:

- `SQR1.L = 0` — sequence length = 1.
- `SQR3.SQ1 = 0` — first (and only) conversion is channel 0 = PA0.
- `SMPR2.SMP0 = 0b111` — 239.5 ADCCLK cycles sample time. Long sample times
  matter for high-impedance sources (a pot's wiper is a worst case).
- Trigger by writing `ADON = 1` again on an already-on ADC. (For trigger by
  timer or external pin, configure `EXTSEL` and use `SWSTART`/external.)
- Spin on `SR.EOC`, then read `DR` (which also clears EOC).

## Print format

```
ADC raw=2047 mV=1649
ADC raw=4095 mV=3300
ADC raw=0 mV=0
```

The mV calc uses `raw * 3300 / 4095` — full-scale 0xFFF → 3300 mV.

## Build & flash

```
make EXAMPLE=07_adc
make EXAMPLE=07_adc flash
```
