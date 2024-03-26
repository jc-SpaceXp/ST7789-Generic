#include "stm32g4xx_spi.h"

#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_hal_gpio.h"

static void spi_gpio_setup(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
	GPIO_InitTypeDef GPIO_InitMiso;
	GPIO_InitMiso.Mode = GPIO_MODE_INPUT;
	//GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Leave floating for now
	GPIO_InitMiso.Pin = SPI_MISO_PIN;
	HAL_GPIO_Init(SPI_MISO_PORT, &GPIO_InitMiso);

	GPIO_InitTypeDef GPIO_InitMosi;
	GPIO_InitMosi.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitMosi.Pin = SPI_MOSI_PIN;
	HAL_GPIO_Init(SPI_MOSI_PORT, &GPIO_InitMosi);

	GPIO_InitTypeDef GPIO_InitClk;
	GPIO_InitClk.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitClk.Pin = SPI_CLK_PIN;
	HAL_GPIO_Init(SPI_CLK_PORT, &GPIO_InitClk);

	GPIO_InitTypeDef GPIO_InitCs;
	GPIO_InitCs.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitCs.Pin = SPI_CS_PIN;
	HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitCs);
}

static void enable_spi(void)
{
	SPI1->CR1 |= SPI_CR1_SPE;
}

void setup_hw_spi(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	spi_gpio_setup();

	// Deafults: (which don't need changing)
	// Clock = Fpclk / 2 (see below)
	// CPHA 0 CPOL 0
	// MSB first
	// CRC disabled
	// Full duplex SPI
	SPI1->CR1 |= LL_SPI_BAUDRATEPRESCALER_DIV16; // test with a slower clock for now
	SPI1->CR1 |= SPI_CR1_MSTR; // STM32 is master
	SPI1->CR1 |= SPI_CR1_SSI; // Handle NSS (CS) via software

	// SSOE ignored as using software NSS
	SPI1->CR2 |= LL_SPI_DATAWIDTH_8BIT;

	// Enable SPI module once setup is complete
	enable_spi();
}

bool tx_complete(void)
{
	bool is_complete = false;
	// For master:
	// BSY bit is set if ongoing tx is occuring
	// (This includes if more data will be sent immediately after due to more data in TXFIFO)
	// so no need to check FTLVL
	bool spi_tx_in_progress = SPI1->SR & SPI_SR_BSY;
	if (!spi_tx_in_progress) {
		is_complete = true;
	}

	return is_complete;
}
