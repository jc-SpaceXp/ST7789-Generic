#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"

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
