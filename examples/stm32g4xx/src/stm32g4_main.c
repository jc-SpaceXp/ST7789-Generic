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

	// Initial modes are:
	// SLPIN, DISPOFF, NORMAL MODE, IDLE OFF

	// Init sequence below
	st7789_power_on_sequence(&st7789, &SPI1->DR);

	st7789_send_command(&st7789, &SPI1->DR, SLPOUT);
	stm32_delay_ms(120);

	// define whole screen region writeable
	unsigned int y_start = 0;
	unsigned int y_end = 319;
	st7789_set_y_coordinates(&st7789, &SPI1->DR, y_start, y_end);

	unsigned int x_start = 0;
	unsigned int x_end = 239;
	st7789_set_x_coordinates(&st7789, &SPI1->DR, x_start, x_end);

	// Needed to display the correct colour, otherwise display is inverted
	st7789_send_command(&st7789, &SPI1->DR, INVON);

	// write a colour to the whole screen, converted to RGB 666
	unsigned int r_col = 170;
	unsigned int g_col = 128;
	unsigned int b_col = 255;
	uint8_t colour_args[3] = { st7789_6bit_colour_index_to_byte(r_col)
	                         , st7789_6bit_colour_index_to_byte(g_col)
	                         , st7789_6bit_colour_index_to_byte(b_col) };
	st7789_send_command(&st7789, &SPI1->DR, RAMWRC);
	for (unsigned int y = 0; y <= y_end; ++y) {
		for (unsigned int x = 0; x <= x_end; ++x) {
			st7789_send_data_via_array(&st7789, &SPI1->DR, colour_args, 3, TxContinue);
		}
	}
	// any command after a N parameter command will stop the previous one
	st7789_send_command(&st7789, &SPI1->DR, DISPON);

	return 0;
}
