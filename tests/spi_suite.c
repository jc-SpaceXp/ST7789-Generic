#include <stdint.h>

#include "greatest.h"
#include "spi_suite.h"

#include "spi.h"

static uint32_t some_gpio_port = 0xFFFFFFFF;

TEST test_write_spi_gpio_pin_0_high(void)
{
	some_gpio_port = 0;
	unsigned int pin = 0;
	assert_spi_pin(&some_gpio_port, pin);
	ASSERT_EQ(some_gpio_port, 1);
	PASS();
}

TEST test_write_spi_gpio_pin_0_low(void)
{
	uint32_t initial_val = 0xFFFFFFFF;
	some_gpio_port = initial_val;
	unsigned int pin = 0; // vals 0-15
	deassert_spi_pin(&some_gpio_port, pin);
	ASSERT_EQ(some_gpio_port, initial_val & ~(1 << pin));
	PASS();
}

TEST test_write_spi_gpio_pin_3_high(void)
{
	uint32_t initial_val = 5;
	some_gpio_port = initial_val;
	unsigned int pin = 3;
	assert_spi_pin(&some_gpio_port, pin);
	ASSERT_EQ(some_gpio_port, initial_val + (1 << pin));
	PASS();
}

SUITE(spi_driver)
{
	RUN_TEST(test_write_spi_gpio_pin_0_high);
	RUN_TEST(test_write_spi_gpio_pin_0_low);
	RUN_TEST(test_write_spi_gpio_pin_3_high);
}

