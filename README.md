# stm32f103-bare-metal

Bare-metal, register-level examples for the STM32F103C8T6 ("Blue Pill"). No HAL, no LL — every peripheral is configured by writing directly to its registers, using the CMSIS header.

## Hardware

- **Board:** Blue Pill (STM32F103C8T6, Cortex-M3, up to 72 MHz, 64 KB Flash, 20 KB RAM)
- **Programmer:** ST-Link v2 (or v2-1, J-Link, CMSIS-DAP) over SWD

  <img width="750" height="500" alt="Image" src="https://github.com/user-attachments/assets/87e84f1b-9ba1-4170-8df7-5eba58a85806" />


## Dependencies

- `arm-none-eabi-gcc`
- `arm-none-eabi-newlib`
- `openocd` (>= 0.8.0)
- `make`

## Layout

```
.
├── inc/         CMSIS headers (Cortex-M3 + STM32F103xB)
├── core/        Shared startup + system init (used by every example)
├── examples/    One folder per example (see table below)
├── build/       Build artifacts (gitignored)
├── Makefile     Parameterized by EXAMPLE=<name>
├── stm32f103c8tx.ld   Linker script (64 KB Flash, 20 KB RAM)
└── openocd.cfg / dbgcfg   OpenOCD config + adapter selector
```

## Examples

| #  | Name             | Peripherals                       |
|----|------------------|-----------------------------------|
| 01 | `01_gpio_blink`  | GPIO                              |
| 02 | `02_systick`     | SysTick                           |
| 03 | `03_uart_tx`     | USART (TX, polled)                |
| 04 | `04_uart_rx_irq` | USART (RX, interrupt + ring buf)  |
| 05 | `05_tim_pwm`     | TIM2 PWM                          |
| 06 | `06_exti_button` | EXTI + NVIC                       |
| 07 | `07_adc`         | ADC1 single conversion            |
| 08 | `08_i2c`         | I2C1 master (bus scanner)         |
| 09 | `09_spi`         | SPI1 master                       |
| 10 | `10_tim_irq`     | TIM3 update interrupt (blink)     |

## Build & flash

Pick a debug adapter once (writes `.interface` for OpenOCD):

```
./dbgcfg
```

Build an example:

```
make EXAMPLE=01_gpio_blink
```

Flash it:

```
make EXAMPLE=01_gpio_blink flash
```

Other targets:

```
make EXAMPLE=01_gpio_blink hex   # Intel HEX
make EXAMPLE=01_gpio_blink bin   # raw binary
make clean                       # remove build/
make list                        # list available examples
make help
```

Override optimization level:

```
make EXAMPLE=01_gpio_blink OPT=-O3
```

Build artifacts are kept per-example under `build/<EXAMPLE>/`, so multiple examples can coexist without `make clean` between switches.
