#include "stm32g4_main.h"
#include "stm32g4xx_spi.h"
#include "spi.h"
#include "st7789.h"

#include "stm32g4xx_hal.h"
//#include "stm32g4xx_nucleo.h"

// User LED for G431KB
#define LD2_PIN  8
#define LD2_BIT0 (1 << 16)
#define LD2_BIT1 (1 << 17)

int main (void)
{
	HAL_Init();

	// Setup LED
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	GPIOB->MODER &= ~((LD2_BIT0) | (LD2_BIT1));
	GPIOB->MODER |= (LD2_BIT0);

	for (;;) {
		assert_spi_pin(&GPIOB->BSRR, LD2_PIN); // set LE2
	}

	return 0;
}
