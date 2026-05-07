# 04 — UART RX (interrupt + ring buffer)

USART1 receive on PA10 with an `RXNE` interrupt service routine that pushes
bytes into a 64-byte ring buffer; `main()` pops bytes and echoes them back.

## Hardware

- **TX pin:** PA9 (USART1_TX, AF push-pull) — for echo
- **RX pin:** PA10 (USART1_RX, input floating)
- **Clock:** HSI 8 MHz, BRR for 9600 baud (see example 03)

## Why a ring buffer

The UART signals `RXNE` for one received byte at a time and the flag must be
cleared (by reading `DR`) within ~1 character time at the configured baud or
else `ORE` is raised and bytes are lost. Doing the byte handling directly in
the foreground would couple input pacing to whatever main() is doing.

The ring buffer here uses the classic single-producer/single-consumer pattern:

- ISR is the *only* writer of `head`.
- `main()` is the *only* writer of `tail`.
- Both indices are `volatile uint32_t`.
- Buffer size is a power of two, so `next = (head + 1) & MASK` replaces a modulo.

This means no critical sections are needed on Cortex-M3: each side updates
its own index after publishing the byte, and torn reads of a 32-bit word do
not happen on aligned 32-bit accesses.

## Register sequence

1. Enable IOPA + USART1 clocks on APB2.
2. PA9 → AF push-pull 50 MHz (TX). PA10 → input floating (RX, default reset state but set explicitly).
3. `USART1->BRR = PCLK2 / 9600`.
4. `USART1->CR1 = UE | TE | RE | RXNEIE` — enable peripheral, both directions, RXNE interrupt.
5. `NVIC_EnableIRQ(USART1_IRQn)` so the NVIC actually delivers the line.

## Error handling

If `ORE`/`FE`/`NE`/`PE` get set without `RXNE` (e.g. line glitches), reading
`DR` after `SR` clears them — the ISR does this so the stream recovers.

## Test

```
$ picocom -b 9600 /dev/ttyUSB0
uart_rx_irq ready, type to echo:
hello
hello
```

## Build & flash

```
make EXAMPLE=04_uart_rx_irq
make EXAMPLE=04_uart_rx_irq flash
```
