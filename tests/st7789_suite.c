#include <stdint.h>

#include "greatest.h"
#include "fff.h"
#include "st7789_suite.h"

#include "st7789.h"
#include "spi.h"

struct PulseCheck {
	uint32_t pin_readback;
	unsigned int hold_time;
};

static uint32_t gpio_port_f = 0xFFFFFFFF;


static struct PulseCheck HwResetCheck[5] = {
	{ 40, 40 }
	, { 40, 40 }
	, { 40, 40 }
	, { 40, 40 }
	, { 40, 40 }
}; 

uint16_t read_gpio_port(uint32_t* port_address)
{
	return *port_address;
}

void hw_reset_spy(unsigned int x)
{
	static int i = 0;
	HwResetCheck[i].pin_readback = read_gpio_port(&gpio_port_f);
	HwResetCheck[i].hold_time = x;
	++i;
}

void fake_delay(unsigned int x)
{
	(void) x;
}


TEST test_st7789_hw_reset(void)
{
	unsigned int res_pin = 0; // vals 0-15
	st7789_hw_reset(&gpio_port_f, res_pin, &hw_reset_spy);
	ASSERT_EQ(HwResetCheck[1].pin_readback, 0);
	ASSERT_GTE(HwResetCheck[1].hold_time, 5);
	PASS();
}

SUITE(st7789_driver)
{
	RUN_TEST(test_st7789_hw_reset);
}

