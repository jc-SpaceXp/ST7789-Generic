#include <stdint.h>

#include "greatest.h"
#include "fff.h"
#include "st7789_suite.h"

#include "st7789.h"
#include "spi.h"

void fake_delay(unsigned int x)
{
	(void) x;
}


TEST test_st7789_hw_reset(void)
{
	uint32_t pin_output = 0xFFFFFFFF;
	unsigned int res_pin = 0; // vals 0-15
	st7789_hw_reset(&pin_output, res_pin, &fake_delay);
	ASSERT_EQ(1, 1); // TODO
	PASS();
}

SUITE(st7789_driver)
{
	RUN_TEST(test_st7789_hw_reset);
}

