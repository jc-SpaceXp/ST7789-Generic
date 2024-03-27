#include "stm32g4xx_spi.h"

#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_hal_gpio.h"

static void spi_gpio_setup(void)
{
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN) | (RCC_AHB2ENR_GPIOBEN);
	// Set all to inputs
	GPIOB->MODER &= ~((GPIO_MODER_MODE3) | (GPIO_MODER_MODE4) | (GPIO_MODER_MODE5));
	GPIOA->MODER &= ~(GPIO_MODER_MODE11);
	// Set outputs (minus MISO)
	GPIOB->MODER |= ((GPIO_MODER_MODE3_0) | (GPIO_MODER_MODE5_0));
	GPIOA->MODER |= (GPIO_MODER_MODE11_0);
	// Set push-pull (leave MISO floating)
	GPIOB->OTYPER |= ((GPIO_OTYPER_OT3) | (GPIO_OTYPER_OT5));
	GPIOA->OTYPER |= (GPIO_OTYPER_OT11);
	// High speed pins
	GPIOB->OSPEEDR |= ((GPIO_OSPEEDR_OSPEED3_1) | (GPIO_OSPEEDR_OSPEED4_1) | (GPIO_OSPEEDR_OSPEED5_1));
	GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED11_1);
	// Clear reset bit on B4, makes B4 floating input
	// Others have no pull-up or pull-down
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD4);
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

bool tx_ready_to_transmit(void)
{
	// TXE = 1 if any data can be sent over without an overrun
	bool is_ready = SPI1->SR & SPI_SR_TXE;
	return is_ready;
}

bool tx_complete(void)
{
	// For master:
	// BSY bit is set if ongoing tx is occuring
	// (This includes if more data will be sent immediately after due to more data in TXFIFO)
	// so no need to check FTLVL
	bool spi_tx_in_progress = SPI1->SR & SPI_SR_BSY;

	return !spi_tx_in_progress;
}
