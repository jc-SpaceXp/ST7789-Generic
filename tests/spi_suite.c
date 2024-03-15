#include <stdint.h>

#include "greatest.h"
#include "spi_suite.h"

#include "spi.h"

TEST test_write_spi_gpio_pin_high(void)
{
	uint32_t pin_output = 0;
	unsigned int pin = 0;
	assert_spi_pin(&pin_output, pin);
	ASSERT_EQ(pin_output, 1);
	PASS();
}

TEST test_write_spi_gpio_pin_low(void)
{
	uint32_t pin_output = 0xFFFFFFFF;
	unsigned int pin = 0; // vals 0-15
	deassert_spi_pin(&pin_output, pin);
	ASSERT_EQ(pin_output, 0);
	PASS();
}

SUITE(spi_driver)
{
	RUN_TEST(test_write_spi_gpio_pin_high);
	RUN_TEST(test_write_spi_gpio_pin_low);
}

