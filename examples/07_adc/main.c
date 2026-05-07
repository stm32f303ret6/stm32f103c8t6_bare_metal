/*
 * 07_adc — single-shot ADC1 conversion on PA0 (channel 0), result printed
 *          over USART1 (9600 8-N-1) on PA9.
 *
 * Wire a 0..3.3 V signal (e.g. potentiometer wiper) to PA0 and read the raw
 * 12-bit value.
 *
 * Clock model:
 *   HCLK = PCLK2 = 8 MHz (HSI default).
 *   ADC prescaler defaults to /2 → ADCCLK = 4 MHz, well under the 14 MHz max.
 *
 * Calibration sequence (RM0008 §11.4):
 *   1. ADON=1 (power on), wait ≥ 1 µs (tSTAB) for the analog block to settle.
 *   2. RSTCAL=1, wait until it self-clears — resets the calibration registers.
 *   3. CAL=1, wait until it self-clears — runs the calibration cycle.
 *   Failing to calibrate before the first conversion gives a worse linearity
 *   error and a permanent offset of a few LSBs.
 *
 * Sample-time SMP0 = 7 → 239.5 ADC cycles per conversion. At 4 MHz ADCCLK
 * that's ~62 µs total (sampling + 12.5 cycles SAR). Plenty of slew tolerance
 * for a high-impedance pot wiper.
 */
#include "stm32f103xb.h"

#define PCLK2_HZ 8000000U
#define BAUD     9600U

/* ---------- USART1 (same as 03_uart_tx) ---------- */

static void uart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
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

static void uart1_puts(const char *s) { while (*s) uart1_putc(*s++); }

static void uart1_putu(uint32_t v)
{
    char buf[11];
    int  n = 0;
    if (v == 0) { uart1_putc('0'); return; }
    while (v) { buf[n++] = '0' + (v % 10); v /= 10; }
    while (n--) uart1_putc(buf[n]);
}

/* ---------- ADC1 ---------- */

static void adc1_init(void)
{
    /* PA0 already has IOPA clock from uart1_init(); enable ADC1 too. */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* PA0 = analog input: CNF0=00, MODE0=00. */
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);

    /* Single conversion on channel 0. */
    ADC1->SQR1 = 0;                 /* L=0 → 1 conversion in the regular sequence */
    ADC1->SQR3 = 0;                 /* SQ1 = channel 0 */

    /* SMP0 = 0b111 → 239.5 cycles sample time on channel 0. */
    ADC1->SMPR2 = (7U << ADC_SMPR2_SMP0_Pos);

    /* Power on, then wait for stabilization. The reference manual specifies
     * a ≥ 1 µs tSTAB; at 8 MHz HCLK this is ≥ 8 cycles. Be generous. */
    ADC1->CR2 = ADC_CR2_ADON;
    for (volatile int i = 0; i < 100; i++) { __asm__("nop"); }

    /* Reset calibration registers, then run calibration. */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) { }
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) { }
}

static uint16_t adc1_read(void)
{
    /* Software start: writing ADON when the ADC is already on starts a
     * regular conversion. (Equivalent to using EXTSEL=SWSTART + SWSTART.) */
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC)) { }
    return (uint16_t)ADC1->DR;      /* reading DR clears EOC */
}

/* ---------- main ---------- */

static void delay(volatile uint32_t n) { while (n--) { __asm__("nop"); } }

int main(void)
{
    uart1_init();
    adc1_init();

    while (1) {
        uint16_t raw = adc1_read();
        /* Convert to millivolts assuming VREF+ = 3.3 V (Blue Pill ties it
         * to 3V3). Q12 → mV: mv = raw * 3300 / 4095. */
        uint32_t mv = ((uint32_t)raw * 3300U) / 4095U;

        uart1_puts("ADC raw=");
        uart1_putu(raw);
        uart1_puts(" mV=");
        uart1_putu(mv);
        uart1_puts("\r\n");

        delay(800000);
    }
}
