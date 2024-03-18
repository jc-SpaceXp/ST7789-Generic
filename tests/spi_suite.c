#include <stdint.h>

#include "greatest.h"
#include "spi_suite.h"

#include "spi.h"

TEST test_write_spi_gpio_pin_0_high(void)
{
	uint32_t pin_output = 0;
	unsigned int pin = 0;
	assert_spi_pin(&pin_output, pin);
	ASSERT_EQ(pin_output, 1);
	PASS();
}

TEST test_write_spi_gpio_pin_0_low(void)
{
	uint32_t pin_output = 0xFFFFFFFF;
	unsigned int pin = 0; // vals 0-15
	deassert_spi_pin(&pin_output, pin);
	ASSERT_EQ(pin_output, 0xFFFFFFFE);
	PASS();
}

TEST test_write_spi_gpio_pin_3_high(void)
{
	uint32_t pin_output = 5;
	unsigned int pin = 3;
	assert_spi_pin(&pin_output, pin); //2^3 is 8
	ASSERT_EQ(pin_output, 13); // 8 + 5, previous pins should still be set
	PASS();
}

SUITE(spi_driver)
{
	RUN_TEST(test_write_spi_gpio_pin_0_high);
	RUN_TEST(test_write_spi_gpio_pin_0_low);
	RUN_TEST(test_write_spi_gpio_pin_3_high);
}

