#include "spi.h"

#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"

void assert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin)
{
	*gpio_output_addr = 1;
	return;
}

void deassert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin)
{
	*gpio_output_addr = 0;
	return;
}

void setup_hw_spi(void)
{
	// Need to configure GPIO pins too
	// TODO

	// Deafults: (which don't need changing)
	// Clock = Fpclk / 2
	// CPHA 0 CPOL 0
	// MSB first
	// CRC disabled
	// Full duplex SPI
	SPI1->CR1 |= SPI_CR1_MSTR; // STM32 is master
	SPI1->CR1 |= SPI_CR1_SSI; // Handle NSS (CS) via software

	// SSOE ignored as using software NSS
	SPI1->CR2 |= LL_SPI_DATAWIDTH_8BIT;

	// Enable SPI module once setup is complete
}

// 5-9 us for RST delay (HW reset)
// Display is then blanked for 120ms
void delay_us(unsigned int hclk_clock_divider)
{
	unsigned int delay_count = 15; // for a 1 us delay (minimum)
	delay_count = 15 * hclk_clock_divider; // for a 1 us delay (minimum)

	TIM2->CR1 |= TIM_CR1_DIR; // Down counter
	TIM2->CNT = delay_count;
	TIM2->PSC = 0;
	TIM2->ARR = delay_count;

	// Enable timer
	TIM2->CR1 |= TIM_CR1_CEN;

	while (!(TIM2->SR & TIM_SR_UIF)) {
		// delay
	}

	TIM2->SR &= ~TIM_SR_UIF;

	// Disable timer
	TIM2->CR1 &= ~TIM_CR1_CEN;
}
