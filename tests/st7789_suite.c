#include <stdint.h>

#include "greatest.h"
#include "fff.h"
#include "st7789_suite.h"

#include "st7789.h"

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(assert_spi_pin, uint32_t*, unsigned int);
FAKE_VOID_FUNC(deassert_spi_pin, uint32_t*, unsigned int);

static uint32_t gpio_port_f = 0xFFFFFFFF;
static uint32_t some_res_addr = 0xFFFFFFFF;
static unsigned int capture_delay = 0;

static struct St7789Internals some_st7789;

static void setup_st7789_struct(void* arg)
{
	RESET_FAKE(assert_spi_pin);
	RESET_FAKE(deassert_spi_pin);
	FFF_RESET_HISTORY();
	capture_delay = 0;

	some_st7789.res_addr = &some_res_addr;
	some_st7789.res_pin = 5;

	(void) arg; // suppress unused warning
}

void fake_delay(unsigned int x)
{
	capture_delay = x;
}

TEST test_st7789_hw_reset(void)
{
	uint32_t initial_val = 0xFFFFFFFF;
	gpio_port_f = initial_val;
	some_st7789.res_pin = 4; // vals 0-15
	st7789_hw_reset(&some_st7789, &fake_delay);
	ASSERT_EQ(fff.call_history[0], (void*) assert_spi_pin);
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin);
	ASSERT_EQ(fff.call_history[2], (void*) assert_spi_pin);
	ASSERT_GTE(capture_delay, 5);
	PASS();
}


SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_struct, NULL);
	RUN_TEST(test_st7789_hw_reset);
}

