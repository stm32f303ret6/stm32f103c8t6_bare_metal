/*
 * 04_uart_rx_irq — USART1 receive via RXNE interrupt + lock-free ring buffer.
 *
 * The ISR pushes incoming bytes; main() pops them and echoes back over the
 * same UART (TX is polled, like example 03). Demonstrates:
 *
 *   - NVIC enable / priority-grouping basics for USART1.
 *   - The single-producer / single-consumer ring buffer pattern that lets
 *     us share state between the ISR and main without disabling interrupts:
 *     the ISR is the only writer of `head`, main() is the only writer of
 *     `tail`, and a power-of-two size makes the modulo a bit-mask.
 *
 * Pins: PA9 = TX (AF push-pull), PA10 = RX (input floating or pull-up).
 *       For RX, "input floating" (CNF=01, MODE=00) is the bare minimum;
 *       in noisy setups prefer "input pull-up" (CNF=10, MODE=00, ODR=1).
 */
#include "stm32f103xb.h"

#define PCLK2_HZ 8000000U
#define BAUD     9600U

/* Power of two so we can mask instead of modulo. */
#define RXBUF_SIZE 64U
#define RXBUF_MASK (RXBUF_SIZE - 1U)

static volatile uint8_t  rx_buf[RXBUF_SIZE];
static volatile uint32_t rx_head;   /* written by ISR  */
static volatile uint32_t rx_tail;   /* written by main */

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    if (sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)USART1->DR;          /* reading DR clears RXNE */
        uint32_t next = (rx_head + 1U) & RXBUF_MASK;
        if (next != rx_tail) {                    /* drop on full */
            rx_buf[rx_head] = b;
            rx_head = next;
        }
    }

    /* If a framing/overrun/noise error occurs, RXNE may not be set and the
     * line stays stuck. Reading SR then DR clears ORE/FE/NE/PE. */
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        (void)USART1->DR;
    }
}

static int rx_pop(uint8_t *out)
{
    if (rx_tail == rx_head) return 0;
    *out = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) & RXBUF_MASK;
    return 1;
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

static void uart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9 (TX) AF push-pull, 50 MHz. */
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |=  GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1;

    /* PA10 (RX) input floating: CNF10=01, MODE10=00. */
    GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |=  GPIO_CRH_CNF10_0;

    USART1->BRR = PCLK2_HZ / BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    NVIC_EnableIRQ(USART1_IRQn);
}

int main(void)
{
    uart1_init();
    uart1_puts("uart_rx_irq ready, type to echo:\r\n");

    while (1) {
        uint8_t c;
        if (rx_pop(&c)) {
            uart1_putc((char)c);
            if (c == '\r') uart1_putc('\n');     /* turn CR into CR/LF */
        }
    }
}
