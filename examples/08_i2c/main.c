/*
 * 08_i2c — I2C1 master bus scanner. Walks 7-bit addresses 0x08..0x77 and
 *          prints any address that ACKs over USART1 at 9600 8-N-1.
 *
 * The scan strategy is the same one `i2cdetect` uses on Linux: send a START
 * + ADDR<<1|W transaction; if the slave ACKs the address byte the device
 * exists, otherwise the AF flag is raised. Either way we send STOP.
 *
 * Bus pins: PB6 = SCL, PB7 = SDA — both AF *open-drain*, 50 MHz.
 * External 4.7 kΩ pull-ups to 3V3 on each line are required (the F1 GPIO
 * "internal pull-up" is far too weak — typically 30..50 kΩ — to drive the
 * bus capacitance at 100 kHz cleanly).
 *
 * Clock model:
 *   PCLK1 = HCLK = 8 MHz (HSI default, APB1 prescaler /1).
 *   I2C kernel clock = PCLK1 = 8 MHz → CR2.FREQ = 8.
 *   Standard-mode (Sm) at 100 kHz: thigh = tlow = 5 µs.
 *     CCR = thigh / Tpclk = 5 µs / 125 ns = 40   (Sm, F/S=0, DUTY=0)
 *     TRISE = (1000 ns / Tpclk) + 1 = 9          (max SCL rise time = 1000 ns in Sm)
 *
 * NOTE: The F1 I2C has well-known errata around clock stretching and the
 * lock-up that follows when SDA is held low at boot. A robust driver
 * normally bit-bangs nine clocks on SCL before enabling the peripheral.
 * For a scan against well-behaved devices we skip that workaround for
 * brevity; if you ever see the bus stuck after a power glitch, that's
 * the first thing to add.
 */
#include "stm32f103xb.h"

#define PCLK1_HZ      8000000U
#define PCLK2_HZ      8000000U
#define BAUD          9600U
#define I2C_TIMEOUT   100000U

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

/* ---------- I2C1 ---------- */

static void i2c1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6, PB7 = AF open-drain, 50 MHz. CRL nibbles bits 24..27 (PB6) and 28..31 (PB7). */
    GPIOB->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6 | GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOB->CRL |=  (GPIO_CRL_CNF6_0 | GPIO_CRL_CNF6_1 | GPIO_CRL_MODE6_0 | GPIO_CRL_MODE6_1)
                 | (GPIO_CRL_CNF7_0 | GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_0 | GPIO_CRL_MODE7_1);

    /* Reset the peripheral, in case a previous run left it in a stuck state. */
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    I2C1->CR2  = (PCLK1_HZ / 1000000U) & I2C_CR2_FREQ;   /* FREQ = 8 MHz */
    I2C1->CCR  = 40U;                                    /* Sm, 100 kHz */
    I2C1->TRISE = (PCLK1_HZ / 1000000U) + 1U;            /* 9 */
    I2C1->CR1  = I2C_CR1_PE;
}

/* Returns 1 if the address at addr7 ACKs a write, 0 if NACK or timeout. */
static int i2c1_ping(uint8_t addr7)
{
    uint32_t t;

    /* START */
    I2C1->CR1 |= I2C_CR1_START;
    for (t = 0; t < I2C_TIMEOUT; t++) if (I2C1->SR1 & I2C_SR1_SB) break;
    if (t == I2C_TIMEOUT) goto fail;

    /* Sending the address clears SB. Reading SR1 first (already done) then
     * writing DR is the documented atomic sequence. */
    I2C1->DR = (uint8_t)((addr7 << 1) | 0U);

    int found = 0;
    for (t = 0; t < I2C_TIMEOUT; t++) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & I2C_SR1_AF) {                  /* slave NACKed */
            I2C1->SR1 &= ~I2C_SR1_AF;            /* W0C: must clear */
            break;
        }
        if (sr1 & I2C_SR1_ADDR) {                /* slave ACKed */
            (void)I2C1->SR1; (void)I2C1->SR2;    /* clear ADDR */
            found = 1;
            break;
        }
    }

fail:
    I2C1->CR1 |= I2C_CR1_STOP;
    /* Wait for the bus to actually go idle before next iteration. */
    for (t = 0; t < I2C_TIMEOUT; t++) if (!(I2C1->SR2 & I2C_SR2_BUSY)) break;
    return found;
}

/* ---------- main ---------- */

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    uart1_init();
    i2c1_init();

    while (1) {
        uart1_puts("I2C scan: ");
        int n = 0;
        for (uint8_t a = 0x08; a <= 0x77; a++) {
            if (i2c1_ping(a)) {
                if (n++) uart1_puts(", ");
                uart1_puts("0x"); uart1_puthex2(a);
            }
        }
        if (!n) uart1_puts("(none)");
        uart1_puts("\r\n");
        delay(2000000);
    }
}
