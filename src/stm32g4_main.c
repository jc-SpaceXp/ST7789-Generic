#include "stm32g4_main.h"
#include "stm32g4xx_spi.h"
#include "stm32g4xx_timers.h"
#include "spi.h"
#include "st7789.h"
#include "st7789_private.h"

#include "stm32g4xx_hal.h"
//#include "stm32g4xx_nucleo.h"

// User LED for G431KB
#define LD2_PIN  8
#define LD2_BIT0 (1 << 16)
#define LD2_BIT1 (1 << 17)

int main (void)
{
	HAL_Init();

	RCC->CFGR |= RCC_CFGR_HPRE_DIV8;
	// Setup LED
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	GPIOB->MODER &= ~((LD2_BIT0) | (LD2_BIT1));
	GPIOB->MODER |= (LD2_BIT0);
	struct St7789SpiPin ld2_pin = { &GPIOB->BSRR, &GPIOB->BRR, LD2_PIN };

	for (;;) {
		assert_spi_pin(ld2_pin.assert_addr, ld2_pin.pin); // set
		HAL_Delay(40);
		assert_spi_pin(ld2_pin.deassert_addr, ld2_pin.pin); // clear
		HAL_Delay(40);
	}

	return 0;
}
