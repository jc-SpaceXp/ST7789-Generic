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

static uint32_t some_gpio_port_b = 0xFFFFFFFF;
static uint32_t some_gpio_port_f = 0xFFFFFFFF;
static uint16_t some_spi_data_reg = 0xFFFF;
static unsigned int capture_delay = 0;

static struct St7789SpiPin some_st7789_pin;
static struct St7789Internals some_st7789;

static void setup_st7789_tests(void* arg)
{
	RESET_FAKE(assert_spi_pin);
	RESET_FAKE(deassert_spi_pin);
	FFF_RESET_HISTORY();
	capture_delay = 0;

	unsigned int reset_pin = 4;
	unsigned int cs_pin = 15;
	unsigned int dcx_pin = 10;

	set_spi_pin_details(&some_st7789_pin
	                   , &some_gpio_port_b
	                   , &some_gpio_port_b
	                   , reset_pin);
	set_st7789_pin_details(&some_st7789, &some_st7789_pin, RSX);
	set_spi_pin_details(&some_st7789_pin
	                   , &some_gpio_port_f
	                   , &some_gpio_port_f
	                   , cs_pin);
	set_st7789_pin_details(&some_st7789, &some_st7789_pin, CSX);
	set_spi_pin_details(&some_st7789_pin
	                   , &some_gpio_port_f
	                   , &some_gpio_port_f
	                   , dcx_pin);
	set_st7789_pin_details(&some_st7789, &some_st7789_pin, DCX);

	initial_st7789_modes(&some_st7789.st7789_mode);

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
	ASSERT_EQ(assert_spi_pin_fake.arg0_history[0], &some_gpio_port_b);
	ASSERT_EQ(assert_spi_pin_fake.arg0_history[1], &some_gpio_port_b);
	ASSERT_EQ(deassert_spi_pin_fake.arg0_history[0], &some_gpio_port_b);
	ASSERT_GTE(capture_delay, 5);
	PASS();
}

TEST test_st7789_sw_reset(void)
{
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	// DC/X needs to be pulled lo (DC/X is an extra GPIO pin)
	// CS must aldo be pulled low when a data needs to be sent or recieved
	ASSERT_EQ(fff.call_history[0], (void*) deassert_spi_pin); // DC/X
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin); // CS
	ASSERT_EQ(fff.call_history[2], (void*) trigger_spi_transfer);
	ASSERT_EQ(fff.call_history[3], (void*) assert_spi_pin); // CS
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[0], 10);
	ASSERT_EQ(deassert_spi_pin_fake.arg0_history[0], &some_gpio_port_f);
	ASSERT_EQ(trigger_spi_transfer_fake.arg1_history[0], 0x01); // 0x01 == SW Reset command
	PASS();
}

TEST st7789_normal_state_before_resets(void)
{
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;
	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_normal_state_after_resets(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET); // SWRESET
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_transition_to_sleep_out_after_resets(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SLPOUT);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepOut);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_transition_sleep_out_to_in(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SLPOUT);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SLPIN);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_transition_normal_to_partial_disp(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	st7789_send_command(&some_st7789, &some_spi_data_reg, PLTON);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), PartialDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}


SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_tests, NULL);
	RUN_TEST(test_st7789_hw_reset);
	RUN_TEST(test_st7789_sw_reset);
	RUN_TEST(st7789_normal_state_before_resets);
	RUN_TEST(st7789_normal_state_after_resets);
	RUN_TEST(st7789_transition_to_sleep_out_after_resets);
	RUN_TEST(st7789_transition_sleep_out_to_in);
	RUN_TEST(st7789_transition_normal_to_partial_disp);
}

