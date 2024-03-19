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
static uint32_t some_res_addr = 0xFFFFFFFF;

static struct St7789Internals some_st7789;

static void setup_st7789_struct(void* arg)
{
	some_st7789.res_addr = &some_res_addr;
	some_st7789.res_pin = 5;

	(void) arg; // suppress unused warning
}


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
	HwResetCheck[i].pin_readback = read_gpio_port(some_st7789.res_addr);
	HwResetCheck[i].hold_time = x;
	++i;
}

void fake_delay(unsigned int x)
{
	(void) x;
}

TEST test_st7789_hw_reset(void)
{
	uint32_t initial_val = 0xFFFFFFFF;
	gpio_port_f = initial_val;
	some_st7789.res_pin = 4; // vals 0-15
	st7789_hw_reset(&some_st7789, &hw_reset_spy);
	ASSERT_EQ_FMT(HwResetCheck[1].pin_readback, (uint16_t) initial_val & ~(1 << some_st7789.res_pin), "%u");
	ASSERT_GTE(HwResetCheck[1].hold_time, 5);
	PASS();
}

SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_struct, NULL);
	RUN_TEST(test_st7789_hw_reset);
}

