#include "stm32g4_main.h"
#include "stm32g4xx_spi.h"
#include "stm32g4xx_timers.h"
#include "spi.h"
#include "st7789.h"
#include "st7789_private.h"

#include "stm32g4xx_hal.h"

// User LED for G431KB
#define LD2_PIN  8
#define LD2_BIT0 (1 << 16)
#define LD2_BIT1 (1 << 17)

int main (void)
{
	unsigned int hclk_divider = 1;
	timer_setup(hclk_divider);

	setup_hw_spi();
	// Can only use deassert_spi_pin with ODR, as BRR and BSRR require 1's to reset
	// deassert will write a zero to a bit pos
	struct St7789SpiPin csx_pin = { &GPIOA->BSRR, &GPIOA->ODR, SPI_CS_PIN };
	struct St7789SpiPin rsx_pin = { &GPIOA->BSRR, &GPIOA->ODR, GPIO_RSX_PIN };
	struct St7789SpiPin dcx_pin = { &GPIOA->BSRR, &GPIOA->ODR, GPIO_DCX_PIN };
	// MISO/MOSI will be TXFIFO and RXFIFO, no need for a struct pin for those
	struct St7789Internals st7789;
	set_st7789_pin_details(&st7789, &csx_pin, CSX);
	set_st7789_pin_details(&st7789, &dcx_pin, DCX);
	set_st7789_pin_details(&st7789, &rsx_pin, RSX);
	initial_st7789_modes(&st7789.st7789_mode);
	const struct UserCallbacksSt7789 st7789_callbacks = {
		&stm32_delay_us
		, &tx_ready_to_transmit
		, &tx_complete
	};
	init_st7789_callbacks(&st7789.user_defined, &st7789_callbacks);

	set_screen_size(&st7789.screen_size, 240, 320);

	// write a colour to the whole screen, converted to RGB 666
	struct RawRgbInput rgb = { 170, 128, 255 };
	st7789_init_sequence(&st7789, &SPI1->DR, InvertOn, FillRegion, rgb);

	return 0;
}
