#include "st7789.h"
#include "st7789_private.h" // for opaque pointer
#include "spi.h"


void set_res_addr(struct St7789Internals* st7789_driver, volatile uint32_t* res_addr)
{
	st7789_driver->res_addr = res_addr;
}

void set_res_pin(struct St7789Internals* st7789_driver, unsigned int res_pin)
{
	st7789_driver->res_pin = res_pin;
}

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int))
{
	// Must be a hi-lo transition, pulse RES for 10us minimum
	// Ignored in sleep-in mode
	assert_spi_pin(st7789_driver->res_addr, st7789_driver->res_pin);
	delay_us(15);
	deassert_spi_pin(st7789_driver->res_addr, st7789_driver->res_pin);
	delay_us(10); // 5-9 us for a valid reset
	assert_spi_pin(st7789_driver->res_addr, st7789_driver->res_pin);
	// Display is then blanked for 120ms
}
