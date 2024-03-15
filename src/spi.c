#include "spi.h"

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
