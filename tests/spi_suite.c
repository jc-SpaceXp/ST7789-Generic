#include <stdint.h>

#include "greatest.h"
#include "spi_suite.h"

#include "spi.h"

TEST test_write_spi_gpio_pin_high(void)
{
	uint32_t output_addr;
	unsigned int pin = 10;
	assert_spi_pin(&output_addr, pin);
	ASSERT_EQ(output_addr, 1);
	PASS();
}

TEST test_write_spi_gpio_pin_low(void)
{
	uint32_t output_addr;
	unsigned int pin = 15; // vals 0-15
	deassert_spi_pin(&output_addr, pin);
	ASSERT_EQ(output_addr, 0);
	PASS();
}

SUITE(spi_driver)
{
	RUN_TEST(test_write_spi_gpio_pin_high);
	RUN_TEST(test_write_spi_gpio_pin_low);
}

