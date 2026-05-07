# 03 — UART TX (polled)

Transmits `Hello, world!\r\n` once per second over USART1 at 9600 8-N-1.
Polled — no interrupts, no DMA.

## Hardware

- **TX pin:** PA9 (USART1_TX, default mapping — no AFIO remap)
- **Receiver:** any 3.3 V serial dongle (CP2102 / CH340 / FT232) GND-referenced to the Blue Pill
- **Clock:** HSI 8 MHz post-reset (no PLL)

> ⚠ The Blue Pill's USART pins are 5 V-tolerant on input but the dongle must drive **3.3 V** logic. Common 5 V FTDI cables can fry the chip on the RX side.

## Register sequence

1. Enable IOPA + USART1 clocks on APB2 (`RCC->APB2ENR`).
2. Configure PA9 as alternate-function push-pull, 50 MHz drive (`CNF9=10`, `MODE9=11` in `GPIOA->CRH`).
3. `USART1->BRR = PCLK2 / baud`. BRR is USARTDIV in 12.4 fixed point, but since `(m<<4)|f == f_pclk/baud` you can just write the integer division directly. With 8 MHz/9600 = 833 (0x341) → ~9604 baud, 0.04 % error.
4. `USART1->CR1 = UE | TE` — enable the peripheral and the transmitter.
5. To send a byte: spin on `SR.TXE`, then write `DR`.

`USART_CR1_M = 0` (default) selects 8 data bits, `USART_CR1_PCE = 0` disables parity, `USART_CR2_STOP = 00` (default) selects 1 stop bit — i.e. 8-N-1.

## Test

```
$ picocom -b 9600 /dev/ttyUSB0
Hello, world!
Hello, world!
...
```

## Build & flash

```
make EXAMPLE=03_uart_tx
make EXAMPLE=03_uart_tx flash
```
