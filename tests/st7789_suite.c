#include <stdint.h>

#include "greatest.h"
#include "fff.h"
#include "st7789_suite.h"

#include "st7789.h"
#include "st7789_private.h"

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(assert_spi_pin, uint32_t*, unsigned int);
FAKE_VOID_FUNC(deassert_spi_pin, uint32_t*, unsigned int);
FAKE_VOID_FUNC(trigger_spi_transfer, uint16_t*, uint16_t);

static uint32_t some_res_addr = 0xFFFFFFFF;
static uint32_t some_dcx_addr = 0xFFFFFFFF;
static uint16_t some_spi_data_reg = 0xFFFF;
static unsigned int capture_delay = 0;

static struct St7789Internals some_st7789;

static void setup_st7789_tests(void* arg)
{
	RESET_FAKE(assert_spi_pin);
	RESET_FAKE(deassert_spi_pin);
	FFF_RESET_HISTORY();
	capture_delay = 0;

	set_res_addr(&some_st7789, &some_res_addr);
	set_res_pin(&some_st7789, 4); // values between 0-15
	set_dcx_addr(&some_st7789, &some_dcx_addr);
	set_dcx_pin(&some_st7789, 10); // values between 0-15

	(void) arg; // suppress unused warning
}

void fake_delay(unsigned int x)
{
	capture_delay = x;
}

TEST test_st7789_hw_reset(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	ASSERT_EQ(fff.call_history[0], (void*) assert_spi_pin);
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin);
	ASSERT_EQ(fff.call_history[2], (void*) assert_spi_pin);
	ASSERT_EQ(assert_spi_pin_fake.arg1_history[0], 4);
	ASSERT_EQ(assert_spi_pin_fake.arg1_history[1], 4);
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[0], 4);
	ASSERT_EQ(assert_spi_pin_fake.arg0_history[0], &some_res_addr);
	ASSERT_EQ(assert_spi_pin_fake.arg0_history[1], &some_res_addr);
	ASSERT_EQ(deassert_spi_pin_fake.arg0_history[0], &some_res_addr);
	ASSERT_GTE(capture_delay, 5);
	PASS();
}

TEST test_st7789_sw_reset(void)
{
	st7789_sw_reset(&some_st7789, &some_spi_data_reg);
	// DC/X needs to be pulled lo (DC/X is an extra GPIO pin)
	// CS must aldo be pulled low when a data needs to be sent or recieved
	ASSERT_EQ(fff.call_history[0], (void*) deassert_spi_pin); // DC/X
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin); // CS
	ASSERT_EQ(fff.call_history[2], (void*) trigger_spi_transfer);
	ASSERT_EQ(fff.call_history[3], (void*) assert_spi_pin); // CS
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[0], 10);
	ASSERT_EQ(deassert_spi_pin_fake.arg0_history[0], &some_dcx_addr);
	ASSERT_EQ(trigger_spi_transfer_fake.arg1_history[0], 0x01); // 0x01 == SW Reset command
	PASS();
}


SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_tests, NULL);
	RUN_TEST(test_st7789_hw_reset);
	RUN_TEST(test_st7789_sw_reset);
}

