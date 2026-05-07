/*
 * 03_uart_tx — USART1 polled transmit at 9600 8-N-1 on PA9.
 *
 * Sends "Hello, world!\r\n" once per second. No interrupts, no DMA — just
 * spin on the TXE flag.
 *
 * Clock model (post-reset, no PLL):
 *   SYSCLK = HCLK = PCLK2 = HSI = 8 MHz, APB2 prescaler /1.
 *   USART1 sits on APB2 so its kernel clock is also 8 MHz.
 *
 *   USARTDIV = f_pclk / (16 * baud)
 *            = 8_000_000 / (16 * 9600)
 *            = 52.0833...
 *
 * BRR is USARTDIV in 12.4 fixed-point — the easy form is just to write the
 * raw integer ratio f_pclk / baud, since (m << 4 | f) == f_pclk / baud:
 *
 *   BRR = 8_000_000 / 9600 = 833 = 0x341
 *
 * That gives an actual baud of 8_000_000/833 = 9603.84, ~0.04 % error.
 *
 * Pin: PA9 (USART1_TX) — alternate-function push-pull, 50 MHz drive.
 *      CNF9 = 0b10 (AF push-pull), MODE9 = 0b11 (output, 50 MHz).
 *      AFIO clock is *not* required here; we are using USART1's default
 *      mapping on PA9/PA10 (no remap), and AFIO is only needed for remap
 *      and EXTI line configuration on F1.
 */
#include "stm32f103xb.h"

#define PCLK2_HZ 8000000U
#define BAUD     9600U

static void uart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9 = AF push-pull, 50 MHz. CRH nibble for pin 9 is bits 4..7. */
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |=  GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1;

    USART1->BRR = PCLK2_HZ / BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void uart1_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE)) { }
    USART1->DR = (uint8_t)c;
}

static void uart1_puts(const char *s)
{
    while (*s) uart1_putc(*s++);
}

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    uart1_init();
    while (1) {
        uart1_puts("Hello, world!\r\n");
        delay(800000);
    }
}
