/*
 * 09_spi — SPI1 master full-duplex byte exchange.
 *
 * Sends the bytes "ABCD" over MOSI and prints whatever appeared on MISO at
 * the same time over USART1. Tie MOSI (PA7) to MISO (PA6) with a jumper to
 * see the loopback echo "ABCD"; otherwise MISO is floating and you'll see
 * undefined data.
 *
 * Pins (default mapping, no AFIO remap):
 *   PA4 = /CS  — manual GPIO output (software NSS, see SSM/SSI below)
 *   PA5 = SCK  — AF push-pull, 50 MHz
 *   PA6 = MISO — input floating
 *   PA7 = MOSI — AF push-pull, 50 MHz
 *
 * Software-NSS rationale: the SPI peripheral has a hardware NSS input that,
 * when low while MSTR=1, forces it back to slave to resolve a "two masters
 * on one bus" condition (MODF). With SSM=1 the peripheral ignores the NSS
 * pin and uses the SSI bit instead — set SSI=1 to keep the peripheral
 * permanently in master mode. We then drive PA4 ourselves around each
 * transaction.
 *
 * Clock model: PCLK2 = 8 MHz. With BR=011 (PCLK/16) → SCK = 500 kHz.
 * CPOL=0/CPHA=0 (SPI mode 0): clock idles low, sample on the leading edge.
 */
#include "stm32f103xb.h"

#define PCLK2_HZ 8000000U
#define BAUD     9600U

/* ---------- USART1 (matches 03_uart_tx) ---------- */

static void uart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |=  GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1;
    USART1->BRR = PCLK2_HZ / BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}
static void uart1_putc(char c) { while (!(USART1->SR & USART_SR_TXE)){} USART1->DR = (uint8_t)c; }
static void uart1_puts(const char *s) { while (*s) uart1_putc(*s++); }
static void uart1_puthex2(uint8_t b)
{
    static const char H[] = "0123456789abcdef";
    uart1_putc(H[(b >> 4) & 0xF]);
    uart1_putc(H[b & 0xF]);
}

/* ---------- SPI1 ---------- */

static void spi1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    /* PA5 (SCK) and PA7 (MOSI): AF push-pull 50 MHz. */
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5
                  | GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |=  (GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_0 | GPIO_CRL_MODE5_1)
                 | (GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_0 | GPIO_CRL_MODE7_1);

    /* PA6 (MISO): input floating. */
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |=  GPIO_CRL_CNF6_0;

    /* PA4 (manual /CS): output push-pull 50 MHz. */
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOA->CRL |=  GPIO_CRL_MODE4_0 | GPIO_CRL_MODE4_1;
    GPIOA->BSRR = GPIO_BSRR_BS4;          /* idle high */

    /* Master, SPI mode 0, BR=011 (fPCLK/16 = 500 kHz), MSB first, 8-bit,
     * software NSS management with SSI driven high to keep MSTR latched. */
    SPI1->CR1 = SPI_CR1_MSTR
              | SPI_CR1_BR_1 | SPI_CR1_BR_0
              | SPI_CR1_SSM  | SPI_CR1_SSI;
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi1_xfer(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = tx;        /* 8-bit access avoids 16-bit framing */
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI1->DR;
}

static void spi1_cs(int low)
{
    GPIOA->BSRR = low ? GPIO_BSRR_BR4 : GPIO_BSRR_BS4;
}

/* ---------- main ---------- */

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    uart1_init();
    spi1_init();

    while (1) {
        const uint8_t tx[] = "ABCD";
        uint8_t rx[4];

        spi1_cs(1);
        for (int i = 0; i < 4; i++) rx[i] = spi1_xfer(tx[i]);
        /* Wait for the last bit to actually shift out before deselecting. */
        while (SPI1->SR & SPI_SR_BSY) { }
        spi1_cs(0);

        uart1_puts("SPI rx:");
        for (int i = 0; i < 4; i++) {
            uart1_putc(' ');
            uart1_puthex2(rx[i]);
        }
        uart1_puts("\r\n");
        delay(1500000);
    }
}
