# 09 — SPI1 master

SPI1 master, mode 0, full-duplex 8-bit transfers at 500 kHz with software
chip-select on PA4. Sends `"ABCD"` over MOSI and prints whatever appears on
MISO at the same time over USART1.

## Hardware

| Signal | Pin  | Mode                            |
|--------|------|---------------------------------|
| /CS    | PA4  | GPIO output push-pull, 50 MHz   |
| SCK    | PA5  | AF push-pull, 50 MHz            |
| MISO   | PA6  | input floating                  |
| MOSI   | PA7  | AF push-pull, 50 MHz            |
| TX     | PA9  | USART1 — 9600 8-N-1 (for output)|

For a quick loopback test, **jumper PA6 ↔ PA7** and the output should read:

```
SPI rx: 41 42 43 44
```

`0x41..0x44` = `'A'..'D'`. Without the jumper MISO floats and the bytes are undefined.

## Software vs hardware NSS

The SPI peripheral has a built-in NSS input. With `MSTR=1` and `SSM=0`, the
master must keep its NSS pin **high** at all times — if NSS goes low it
interprets that as "another master is on the bus", clears `MSTR`, and sets
`MODF`. This is fine if you tie NSS high externally, but it's a pain in
practice.

`SSM=1` says "I'll manage NSS in software": the peripheral reads `SSI` from
`CR1` instead of the pin. Setting `SSI=1` keeps it permanently in master.
You then drive a regular GPIO (PA4 here) yourself around each transaction.

## Clock math

```
SPI clock = PCLK2 / 2^(BR+1)
PCLK2 = 8 MHz, BR = 011 → /16 → SCK = 500 kHz
```

`CR1.BR` valid values are `000..111` → `/2..../256`. The hard upper limit
is `PCLK / 2`, even when SPI is hosted on the slower APB.

## Mode 0 (CPOL=0, CPHA=0)

- SCK idles low.
- Data is **sampled on the leading (rising) edge**, shifted on the trailing edge.
- Most generic peripherals (74HC595 shift registers, MCP3008 ADCs, many DAC chips) speak this. Display controllers (ST7735, SSD1351) often want mode 3.

## Single-byte exchange

```c
while (!(SPI1->SR & SPI_SR_TXE)) {}     // tx FIFO ready
*(volatile uint8_t*)&SPI1->DR = tx;     // 8-bit write keeps DFF=8 framing
while (!(SPI1->SR & SPI_SR_RXNE)) {}    // received the response byte
rx = *(volatile uint8_t*)&SPI1->DR;     // reading clears RXNE
```

The 8-bit-typed access matters: a 32-bit `SPI1->DR = tx` is interpreted
as a 16-bit frame on F1 because the data register is 16-bit-wide and
the compiler may emit a `strh`. Byte access keeps frames 8-bit even with
`DFF=0` (which is the reset default for 8-bit).

After the last byte, **wait for `BSY=0`** before deasserting /CS — `RXNE`
goes high one PCLK before the line stops toggling.

## Build & flash

```
make EXAMPLE=09_spi
make EXAMPLE=09_spi flash
```
