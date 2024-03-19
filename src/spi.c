#include "spi.h"

enum {LAST_PIN = 15};

// Pin is zero indexed
static uint32_t pin_to_bit_pos_conversion(unsigned int pin)
{
	return (1 << pin);
}

void assert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin)
{
	if (gpio_pin > LAST_PIN) { return; }
	*gpio_output_addr |= pin_to_bit_pos_conversion(gpio_pin);
}

void deassert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin)
{
	if (gpio_pin > LAST_PIN) { return; }
	*gpio_output_addr &=  ~pin_to_bit_pos_conversion(gpio_pin);
}
