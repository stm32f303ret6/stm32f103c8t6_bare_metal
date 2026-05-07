# 08 — I2C1 master (bus scanner)

I2C1 master in standard mode (100 kHz). Walks 7-bit addresses 0x08..0x77 and
prints any address that ACKs over USART1, the same way `i2cdetect` works.

## Hardware

- **SCL:** PB6 — AF open-drain, 50 MHz, **external 4.7 kΩ pull-up to 3V3**
- **SDA:** PB7 — AF open-drain, 50 MHz, **external 4.7 kΩ pull-up to 3V3**
- **TX:**  PA9 (USART1) → 3.3 V serial dongle, 9600 8-N-1
- **Clock:** HSI 8 MHz post-reset

> The internal GPIO pull-ups (~30..50 kΩ) are **not** enough for I2C even at 100 kHz — bus capacitance pushes the rise time past the spec'd 1 µs. Always use external pull-ups.

## Clock math (Standard mode)

```
PCLK1 = 8 MHz                        → CR2.FREQ = 8 (in MHz)
SCL  = 100 kHz, thigh = tlow = 5 µs
CCR  = thigh / Tpclk = 5 µs / 125 ns = 40    (Sm, F/S=0, DUTY=0)
TRISE = (1000 ns / Tpclk) + 1 = 9            (Sm spec: tr ≤ 1000 ns)
```

For Fast mode (400 kHz), set `CCR.F/S = 1`, recompute `CCR` (with `DUTY=0` use ratio 2:1, with `DUTY=1` use 16:9), and set `TRISE = (300 ns / Tpclk) + 1`.

## Register sequence

1. Enable IOPB on APB2, I2C1 on APB1.
2. PB6/PB7 → AF open-drain 50 MHz (`CNF=11`, `MODE=11`).
3. **Software reset** the peripheral (`CR1.SWRST = 1` then `= 0`) to recover from any stuck state from a previous run.
4. `CR2.FREQ = 8` (PCLK1 in MHz — the I2C peripheral needs to know this to time SDA setup vs SCL).
5. `CCR = 40`, `TRISE = 9` (Sm @ 100 kHz from 8 MHz).
6. `CR1.PE = 1` to enable.

## Address transfer

For each address `a`:

1. `CR1.START = 1` → wait `SR1.SB`.
2. Write `(a << 1) | 0` to `DR` (7-bit address with the R/W bit clear).
3. Poll `SR1`:
   - `AF` set → slave NACKed → no device. Clear `AF` (write 0).
   - `ADDR` set → slave ACKed → device exists. Clear `ADDR` by reading SR1 then SR2.
4. `CR1.STOP = 1` → wait `SR2.BUSY = 0`.

## Lock-up workaround (mentioned, not implemented)

If the SCL or SDA line gets stuck low at boot (a slave caught mid-transfer
when the master reset), the I2C peripheral cannot recover on its own. The
canonical fix is to bit-bang nine clocks on SCL with both lines reconfigured
as GPIO open-drain, then re-enter I2C mode. Add this if you see hangs after
a reset during a transfer.

## Output

```
I2C scan: 0x3c, 0x68
I2C scan: 0x3c, 0x68
```

…would be a typical bus with an SSD1306 OLED at 0x3c and an MPU6050 IMU at 0x68. With nothing wired you'll see `(none)`.

## Build & flash

```
make EXAMPLE=08_i2c
make EXAMPLE=08_i2c flash
```
