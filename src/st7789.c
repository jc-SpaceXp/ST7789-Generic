#include "st7789.h"
#include "spi.h"

void st7789_hw_reset(uint32_t* res_addr, unsigned int res_pin, void (*delay_us)(unsigned int))
{
	// Must be a hi-lo transition, pulse RES for 10us minimum
	// Ignored in sleep-in mode
	assert_spi_pin(res_addr, res_pin);
	delay_us(15);
	deassert_spi_pin(res_addr, res_pin);
	delay_us(10); // 5-9 us for a valid reset
	assert_spi_pin(res_addr, res_pin);
	// Display is then blanked for 120ms
}
