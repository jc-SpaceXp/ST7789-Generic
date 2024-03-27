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
FAKE_VALUE_FUNC(bool, tx_complete);
FAKE_VALUE_FUNC(bool, tx_ready_to_transmit);

static uint32_t some_gpio_port_b = 0xFFFFFFFF;
static uint32_t some_gpio_port_f = 0xFFFFFFFF;
static uint32_t some_spi_data_reg = 0xFFFF;
static unsigned int capture_delay = 0;

static struct St7789SpiPin some_st7789_pin;
static struct St7789Internals some_st7789;
struct LoopTestSt7789Modes {
	uint8_t command_id;
	struct St7789Modes init_modes;
	struct St7789Modes expected_modes;
};


static void init_st7789_modes_from_struct(struct St7789Modes* st7789_dest
                                         , struct St7789Modes st7789_src)
{
	*st7789_dest = st7789_src;
}

static void setup_st7789_common_tests(void)
{
	RESET_FAKE(assert_spi_pin);
	RESET_FAKE(deassert_spi_pin);
	RESET_FAKE(trigger_spi_transfer);
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

	tx_complete_fake.return_val = true; // Avoid infinite loops
	tx_ready_to_transmit_fake.return_val = true; // Avoid infinite loops
}

static void setup_st7789_tests(void* arg)
{
	setup_st7789_common_tests();
	initial_st7789_modes(&some_st7789.st7789_mode);

	(void) arg; // suppress unused warning
}

static void setup_st7789_transition_tests(void* arg)
{
	setup_st7789_common_tests();
	(void) arg; // suppress unused warning
}

static enum greatest_test_res expected_sleep_mode(struct St7789Modes expected
                                                 , struct St7789Modes actual)
{
	ASSERT_EQ(get_current_sleep_mode(actual), get_current_sleep_mode(expected));
	PASS();
}

static enum greatest_test_res expected_display_mode(struct St7789Modes expected
                                                   , struct St7789Modes actual)
{
	ASSERT_EQ(get_current_display_mode(actual), get_current_display_mode(expected));
	PASS();
}

static enum greatest_test_res expected_idle_mode(struct St7789Modes expected
                                                , struct St7789Modes actual)
{
	ASSERT_EQ(get_current_idle_mode(actual), get_current_idle_mode(expected));
	PASS();
}

static enum greatest_test_res expected_display_on(struct St7789Modes expected
                                                 , struct St7789Modes actual)
{
	ASSERT_EQ(display_is_on(actual), display_is_on(expected));
	PASS();
}

void fake_delay(unsigned int x)
{
	capture_delay = x;
}

TEST snprintf_return_val(bool sn_error)
{
	ASSERT_FALSE(sn_error);
	PASS();
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
	ASSERT_EQ(fff.call_history[2], (void*) tx_ready_to_transmit);
	ASSERT_EQ(fff.call_history[3], (void*) trigger_spi_transfer);
	ASSERT_EQ(fff.call_history[4], (void*) tx_complete);
	ASSERT_EQ(fff.call_history[5], (void*) assert_spi_pin); // CS
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
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_transition_display_off_to_on(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	st7789_send_command(&some_st7789, &some_spi_data_reg, DISPON);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), true);
	PASS();
}

TEST st7789_transition_display_on_to_off(void)
{
	st7789_hw_reset(&some_st7789, &fake_delay);
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);
	st7789_send_command(&some_st7789, &some_spi_data_reg, DISPON);
	st7789_send_command(&some_st7789, &some_spi_data_reg, DISPOFF);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	ASSERT_EQ(get_current_sleep_mode(some_st7789_modes), SleepIn);
	ASSERT_EQ(get_current_display_mode(some_st7789_modes), NormalDisp);
	ASSERT_EQ(get_current_idle_mode(some_st7789_modes), false);
	ASSERT_EQ(display_is_on(some_st7789_modes), false);
	PASS();
}

TEST st7789_transition_generic(const struct LoopTestSt7789Modes* test_st7789_modes)
{
	st7789_send_command(&some_st7789, &some_spi_data_reg, test_st7789_modes->command_id);
	struct St7789Modes some_st7789_modes = some_st7789.st7789_mode;

	CHECK_CALL(expected_sleep_mode(test_st7789_modes->expected_modes, some_st7789_modes));
	CHECK_CALL(expected_display_mode(test_st7789_modes->expected_modes, some_st7789_modes));
	CHECK_CALL(expected_idle_mode(test_st7789_modes->expected_modes, some_st7789_modes));
	CHECK_CALL(expected_display_on(test_st7789_modes->expected_modes, some_st7789_modes));
	PASS();
}

void loop_test_all_transitions(void)
{
	struct LoopTestSt7789Modes transition_modes[24] = {
		//         Init                                  Expected result
		{ SLPOUT, {SleepIn, NormalDisp, false, false},  {SleepOut, NormalDisp, false, false} }
		,{SLPOUT, {SleepIn, NormalDisp, true, true},    {SleepOut, NormalDisp, true, true} }
		,{SLPOUT, {SleepIn, PartialDisp, false, false}, {SleepOut, PartialDisp, false, false} }
		,{SLPOUT, {SleepIn, PartialDisp, true, true},   {SleepOut, PartialDisp, true, true} }

		,{SLPIN, {SleepOut, NormalDisp, false, false},  {SleepIn, NormalDisp, false, false} }
		,{SLPIN, {SleepOut, NormalDisp, true, false},   {SleepIn, NormalDisp, true, false} }
		,{SLPIN, {SleepOut, PartialDisp, false, false}, {SleepIn, PartialDisp, false, false} }
		,{SLPIN, {SleepOut, PartialDisp, true, false},  {SleepIn, PartialDisp, true, false} }

		,{PLTON, {SleepIn, NormalDisp, false, false},   {SleepIn, PartialDisp, false, false} }
		,{PLTON, {SleepIn, NormalDisp, true, false},    {SleepIn, PartialDisp, true, false} }
		,{PLTON, {SleepOut, NormalDisp, false, false},  {SleepOut, PartialDisp, false, false} }
		,{PLTON, {SleepOut, NormalDisp, true, false},   {SleepOut, PartialDisp, true, false} }

		,{NORON, {SleepIn, PartialDisp, false, false},  {SleepIn, NormalDisp, false, false} }
		,{NORON, {SleepIn, PartialDisp, true, false},   {SleepIn, NormalDisp, true, false} }
		,{NORON, {SleepOut, PartialDisp, false, false}, {SleepOut, NormalDisp, false, false} }
		,{NORON, {SleepOut, PartialDisp, true, false},  {SleepOut, NormalDisp, true, false} }

		,{IDMON, {SleepIn, NormalDisp, false, false},   {SleepIn, NormalDisp, true, false} }
		,{IDMON, {SleepIn, PartialDisp, false, false},  {SleepIn, PartialDisp, true, false} }
		,{IDMON, {SleepOut, NormalDisp, false, false},  {SleepOut, NormalDisp, true, false} }
		,{IDMON, {SleepOut, PartialDisp, false, false}, {SleepOut, PartialDisp, true, false} }

		,{IDMOFF, {SleepIn, NormalDisp, true, false},   {SleepIn, NormalDisp, false, false} }
		,{IDMOFF, {SleepIn, PartialDisp, true, false},  {SleepIn, PartialDisp, false, false} }
		,{IDMOFF, {SleepOut, NormalDisp, true, false},  {SleepOut, NormalDisp, false, false} }
		,{IDMOFF, {SleepOut, PartialDisp, true, false}, {SleepOut, PartialDisp, false, false} }
	};

	for (int i = 0; i < 24; ++i) {
		init_st7789_modes_from_struct(&some_st7789.st7789_mode
		                             , transition_modes[i].init_modes);
		char test_suffix[5];
		int sn = snprintf(test_suffix, 4, "%u", i);
		bool sn_error = (sn > 5) || (sn < 0);
		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(snprintf_return_val, sn_error);

		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(st7789_transition_generic, &transition_modes[i]);
	}
}

TEST test_st7789_commands_with_one_arg(void)
{
	st7789_send_data(&some_st7789, &some_spi_data_reg, 0x02); // args: 1st == upper byte

	ASSERT_EQ(fff.call_history[0], (void*) assert_spi_pin); // DC/X high for data
	ASSERT_EQ(assert_spi_pin_fake.arg1_history[0], 10); // 10 == DC/X pin

	PASS();
}

TEST test_st7789_commands_with_four_args(void)
{
	unsigned int cols = 60;
	unsigned int cole = 69;
	uint8_t caset_args[4] = {get_upper_byte(cols), get_lower_byte(cols)
	                        ,get_upper_byte(cole), get_lower_byte(cole)};
	st7789_send_data_via_array(&some_st7789, &some_spi_data_reg, caset_args, 4);

	ASSERT_EQ(trigger_spi_transfer_fake.call_count, 4);
	ASSERT_EQ(assert_spi_pin_fake.arg1_history[0], 10); // 10 == DC/X pin

	PASS();
}

TEST test_st7789_write_18_bit_colour_to_specific_pixel(void)
{
	// CASET and RASET would have been called before
	unsigned int r_col = 0x14; // 01 0100: 6 --> 0101 0000 8 bits
	unsigned int g_col = 0x0A; // 00 1010: 6 --> 0010 1000 8 bits
	unsigned int b_col = 0x85; // 00 0101: 6 --> 0001 0100 8 bits
	// 01 0100 0010 1000 0101: 0x14285
	unsigned int frame_18_bit_col = (r_col << 12) | (g_col << 6) | b_col;
	ASSERT_EQ(frame_18_bit_col, 0x14285); // what the internal frame mem should read (666 RGB)
	// For 18-bit colour the 666 RGB format will be sent over as 3 bytes
	// with each colour having the lowest two bits padded with zero's
	uint8_t colour_args[3] = { st7789_6bit_colour_index_to_byte(r_col)
                             , st7789_6bit_colour_index_to_byte(g_col)
                             , st7789_6bit_colour_index_to_byte(b_col) };
	st7789_send_data_via_array(&some_st7789, &some_spi_data_reg, colour_args, 3);


	ASSERT_EQ(trigger_spi_transfer_fake.call_count, 3);
	ASSERT_EQ(trigger_spi_transfer_fake.arg1_history[0], 0x50);
	ASSERT_EQ(trigger_spi_transfer_fake.arg1_history[1], 0x28);
	ASSERT_EQ(trigger_spi_transfer_fake.arg1_history[2], 0x14);
	PASS();
}


SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_tests, NULL);
	RUN_TEST(test_st7789_hw_reset);
	RUN_TEST(test_st7789_sw_reset);
	RUN_TEST(st7789_normal_state_before_resets);
	RUN_TEST(st7789_normal_state_after_resets);
	RUN_TEST(st7789_transition_display_off_to_on);
	RUN_TEST(st7789_transition_display_on_to_off);
	RUN_TEST(test_st7789_commands_with_one_arg);
	RUN_TEST(test_st7789_commands_with_four_args);
	RUN_TEST(test_st7789_write_18_bit_colour_to_specific_pixel);
}

SUITE(st7789_driver_modes_transitions)
{
	GREATEST_SET_SETUP_CB(setup_st7789_transition_tests, NULL);

	loop_test_all_transitions(); // transitions shown in Sitronix datasheet
}

