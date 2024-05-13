#include <stdint.h>

#include "greatest.h"
#include "fff.h"
#include "st7789_suite.h"

#include "st7789.h"
#include "st7789_private.h"

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(assert_spi_pin, uint32_t*, unsigned int);
FAKE_VOID_FUNC(deassert_spi_pin, uint32_t*, unsigned int);
FAKE_VOID_FUNC(trigger_spi_transfer, uint32_t*, uint16_t);
FAKE_VOID_FUNC(trigger_spi_byte_transfer, uint32_t*, uint8_t);
FAKE_VALUE_FUNC(bool, tx_complete);
FAKE_VALUE_FUNC(bool, tx_ready_to_transmit);

static uint32_t some_gpio_port_b = 0xFFFFFFFF;
static uint32_t some_gpio_port_f = 0xFFFFFFFF;
static uint32_t some_spi_data_reg = 0xFFFF;
static unsigned int capture_delay = 0;

static struct St7789SpiPin some_st7789_pin;
static struct St7789Internals some_st7789;
static struct St7789Size some_st7789_size;

struct LoopTestSt7789Init {
	struct {
		enum InitInversion invert;
		enum FillScreenRegion screen_region;
		struct RawRgbInput rgb;
	};
	// INVON, CASET, RASET
	// (SWRESET, SLPOUT, DISPON are always called hence not needed here)
	struct St7789ExpectedTx {
		uint8_t tx_byte;
		bool tx_expected;
	} tx[4];
};

struct LoopTestSt7789Modes {
	uint8_t command_id;
	struct St7789Modes init_modes;
	struct St7789Modes expected_modes;
};

struct LoopTestSt7789ColourFormats {
	enum BitsPerPixel bpp;
	uint8_t tx_expected;
};

struct LoopTestSt7789FillColour {
	struct Inputs {
		struct Pixels {
			unsigned int total_x;
			unsigned int total_y;
		} pixels;
		struct RawRgbInput rgb;
	} input;
	unsigned int starting_block_index;
};

enum RasetCasetStartOrEnd { Start, End };


static void init_st7789_modes_from_struct(struct St7789Modes* st7789_dest
                                         , struct St7789Modes st7789_src)
{
	*st7789_dest = st7789_src;
}

void fake_delay(unsigned int x)
{
	capture_delay = x;
}

bool fake_tx_always_return_true(void)
{
	return true; // Avoid infinite loops
}

static void setup_st7789_common_tests(void)
{
	RESET_FAKE(assert_spi_pin);
	RESET_FAKE(deassert_spi_pin);
	RESET_FAKE(trigger_spi_transfer);
	RESET_FAKE(trigger_spi_byte_transfer);
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

	set_screen_size(&some_st7789.screen_size, 240, 240);

	const struct UserCallbacksSt7789 some_st7789_callbacks = {
		&fake_delay
		, &fake_tx_always_return_true
		, &fake_tx_always_return_true
	};

	init_st7789_callbacks(&some_st7789.user_defined, &some_st7789_callbacks);
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

static enum greatest_test_res check_hw_reset_call_history(void)
{
	ASSERT_EQ((void*) assert_spi_pin, fff.call_history[0]);
	ASSERT_EQ((void*) deassert_spi_pin, fff.call_history[1]);
	ASSERT_EQ((void*) assert_spi_pin, fff.call_history[2]);
	PASS();
}

static unsigned int hw_reset_fff_call_count(void)
{
	return 3;
}

static enum greatest_test_res check_hw_reset_arg_history(void)
{
	ASSERT_EQ(some_st7789.rsx.pin, assert_spi_pin_fake.arg1_history[0]);
	ASSERT_EQ(some_st7789.rsx.pin, assert_spi_pin_fake.arg1_history[1]);
	ASSERT_EQ(some_st7789.rsx.pin, deassert_spi_pin_fake.arg1_history[0]);
	ASSERT_EQ(&some_gpio_port_b, assert_spi_pin_fake.arg0_history[0]);
	ASSERT_EQ(&some_gpio_port_b, assert_spi_pin_fake.arg0_history[1]);
	ASSERT_EQ(&some_gpio_port_b, deassert_spi_pin_fake.arg0_history[0]);
	ASSERT_GTE(capture_delay, 5); // 5us delay
	PASS();
}

static enum greatest_test_res check_command_call_history(unsigned int start)
{
	// CS must aldo be pulled low when a data needs to be sent or recieved
	ASSERT_EQ((void*) deassert_spi_pin, fff.call_history[start]); // DC/X
	ASSERT_EQ((void*) deassert_spi_pin, fff.call_history[start + 1]); // CS
	ASSERT_EQ((void*) trigger_spi_byte_transfer, fff.call_history[start + 2]);
	ASSERT_EQ((void*) assert_spi_pin, fff.call_history[start + 3]); // CS
	PASS();
}

static enum greatest_test_res check_command_arg_history(unsigned int start)
{
	// DC/X needs to be pulled lo for commands
	// CS is also pulled lo to inidacte the beggining of a transfer
	ASSERT_EQ(some_st7789.dcx.pin, deassert_spi_pin_fake.arg1_history[start]);
	ASSERT_EQ(some_st7789.dcx.deassert_addr, deassert_spi_pin_fake.arg0_history[start]);
	ASSERT_EQ(some_st7789.csx.pin, deassert_spi_pin_fake.arg1_history[start + 1]);
	PASS();
}

static enum greatest_test_res check_data_call_history(unsigned int start)
{
	// CS must also be pulled low when a data needs to be sent or recieved
	ASSERT_EQ((void*) assert_spi_pin, fff.call_history[start]); // DC/X
	ASSERT_EQ((void*) deassert_spi_pin, fff.call_history[start + 1]); // CS
	ASSERT_EQ((void*) trigger_spi_byte_transfer, fff.call_history[start + 2]);
	PASS();
}

static enum greatest_test_res check_spi_pins_data_arg_history(unsigned int start)
{
	// DC/X needs to be pulled hi for commands
	// CS is also pulled lo to inidacte the beggining of a transfer
	ASSERT_EQ(some_st7789.dcx.pin, assert_spi_pin_fake.arg1_history[start]);
	ASSERT_EQ(some_st7789.dcx.assert_addr, assert_spi_pin_fake.arg0_history[start]);
	ASSERT_EQ(some_st7789.csx.pin, deassert_spi_pin_fake.arg1_history[start]);
	PASS();
}

static enum greatest_test_res check_data_initial_spi_behaviour(unsigned int start)
{
	CHECK_CALL(check_data_call_history(start));
	CHECK_CALL(check_spi_pins_data_arg_history(start));
	PASS();
}

static enum greatest_test_res check_data_tx_paused(unsigned int data_start
                                                  , unsigned int previous_spi_tx_calls)
{
	// DC/X already lo, CS/X already lo (+2 below)
	// +X previous spi tx calls
	ASSERT_EQ((void*) assert_spi_pin, fff.call_history[data_start + 2 + previous_spi_tx_calls]);
	ASSERT_EQ(some_st7789.csx.pin, assert_spi_pin_fake.arg1_history[data_start + 1]);
	PASS();
}

static enum greatest_test_res check_tx_byte(uint8_t tx_byte, unsigned int start)
{
	ASSERT_EQ(tx_byte, trigger_spi_byte_transfer_fake.arg1_history[start]);
	PASS();
}

static enum greatest_test_res tx_byte_was_sent(uint8_t tx_byte, bool expected)
{
	bool tx_byte_detected = false;
	for (int i = 0; i < (int) trigger_spi_byte_transfer_fake.call_count; ++i) {
		if (trigger_spi_byte_transfer_fake.arg1_history[i] == tx_byte) {
			tx_byte_detected = true;
			break;
		}
	}
	ASSERT(tx_byte_detected == expected);
	PASS();
}

static enum greatest_test_res check_repeated_tx_data(unsigned int start
                                                    , uint8_t* expected_stream
                                                    , unsigned int length)
{
	for (unsigned int i = 0; i < length; ++i) {
		ASSERT_EQ_FMT(expected_stream[i]
		             , trigger_spi_byte_transfer_fake.arg1_history[start + i]
		             , "%.2X");
	}
	PASS();
}

static enum greatest_test_res check_raset_caset_args(unsigned int start
                                                    , uint8_t expected_byte
                                                    , enum RasetCasetStartOrEnd start_or_end)
{
	unsigned int arg_offset = 1; // args are after command
	if (start_or_end == End) {
		arg_offset += 2;
	}

	// Each arg is split into upper and lower byte
	// args[0-3]: upper(start), lower(start), upper(end), lower(end)
	uint8_t tx_arg = 0;
	tx_arg |= trigger_spi_byte_transfer_fake.arg1_history[start + arg_offset] << 8;
	tx_arg |= trigger_spi_byte_transfer_fake.arg1_history[start + arg_offset + 1];

	ASSERT_EQ_FMT(expected_byte, tx_arg, "%.2X");
	PASS();
}

static int get_first_command_id_index(uint8_t command_id)
{
	int first_cmd_index = -5;
	for (int i = 0; i < (int) trigger_spi_byte_transfer_fake.call_count; ++i) {
		if (trigger_spi_byte_transfer_fake.arg1_history[i] == command_id) {
			first_cmd_index = i;
			break;
		}
	}

	return first_cmd_index;
}


TEST snprintf_return_val(bool sn_error)
{
	ASSERT_FALSE(sn_error);
	PASS();
}


TEST test_st7789_pre_transfer_setup_for_commands(void)
{
	pre_st7789_transfer(&some_st7789, TxCmd);
	ASSERT_EQ(fff.call_history[0], (void*) deassert_spi_pin);
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin);
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[0], 10); // DCX pin
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[1], 15); // CS pin
	PASS();
}

TEST test_st7789_pre_transfer_setup_for_data(void)
{
	pre_st7789_transfer(&some_st7789, TxData);
	ASSERT_EQ(fff.call_history[0], (void*) assert_spi_pin);
	ASSERT_EQ(fff.call_history[1], (void*) deassert_spi_pin);
	ASSERT_EQ(assert_spi_pin_fake.arg1_history[0], 10); // DCX pin
	ASSERT_EQ(deassert_spi_pin_fake.arg1_history[0], 15); // CS pin
	PASS();
}

TEST test_st7789_hw_reset(void)
{
	st7789_hw_reset(&some_st7789);

	CHECK_CALL(check_hw_reset_call_history());
	CHECK_CALL(check_hw_reset_arg_history());
	PASS();
}

TEST test_st7789_sw_reset(void)
{
	unsigned int previous_commands = 0;
	st7789_send_command(&some_st7789, &some_spi_data_reg, SWRESET);

	CHECK_CALL(check_command_call_history(previous_commands));
	CHECK_CALL(check_command_arg_history(previous_commands));
	CHECK_CALL(check_tx_byte(0x01, previous_commands)); // 0x01 == SW Reset command
	PASS();
}

TEST test_st7789_power_on_sequence(void)
{
	unsigned int previous_tx_commands = 0;
	st7789_power_on_sequence(&some_st7789, &some_spi_data_reg);

	CHECK_CALL(check_hw_reset_call_history());
	CHECK_CALL(check_hw_reset_arg_history());
	CHECK_CALL(check_command_call_history(hw_reset_fff_call_count()));
	CHECK_CALL(check_command_arg_history(1));
	CHECK_CALL(check_tx_byte(0x01, previous_tx_commands)); // 0x01 == SW Reset command
	PASS();
}

TEST test_st7789_screen_size(void)
{
	unsigned int x_width = 320;
	unsigned int y_width = 240;
	set_screen_size(&some_st7789_size, x_width, y_width);

	ASSERT_EQ(x_width, some_st7789_size.x);
	ASSERT_EQ(y_width, some_st7789_size.y);
	PASS();
}

TEST test_st7789_init_sequence(const struct LoopTestSt7789Init* st7789_init)
{
	struct St7789Size init_xy_size = { 4, 2 };
	st7789_init_sequence(&some_st7789, &some_spi_data_reg
	                    , st7789_init->invert
	                    , st7789_init->screen_region
	                    , init_xy_size
	                    , st7789_init->rgb);

	ASSERTm("Exceeded max calls to faked function, cannot loop through complete history"
	 , trigger_spi_byte_transfer_fake.call_count < FFF_CALL_HISTORY_LEN);
	ASSERTm("Cannot loop through complete history, some arguments haven't been stored"
	 , trigger_spi_byte_transfer_fake.arg_histories_dropped == 0);
	ASSERT_EQ_FMT(init_xy_size.x, some_st7789.screen_size.x, "%u");
	ASSERT_EQ_FMT(init_xy_size.y, some_st7789.screen_size.y, "%u");
	CHECK_CALL(check_hw_reset_call_history());
	CHECK_CALL(check_hw_reset_arg_history());
	CHECK_CALL(check_command_call_history(hw_reset_fff_call_count()));
	CHECK_CALL(check_command_arg_history(1));
	CHECK_CALL(tx_byte_was_sent(SWRESET, true));
	CHECK_CALL(tx_byte_was_sent(SLPOUT, true));
	CHECK_CALL(tx_byte_was_sent(DISPON, true));
	for (int i = 0; i < 4; ++i) { // tx struct array size is 4
		// INVON, CASET, RASET, RAMWRC
		CHECK_CALL(tx_byte_was_sent(st7789_init->tx[i].tx_byte, st7789_init->tx[i].tx_expected));
	}
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
	st7789_hw_reset(&some_st7789);
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
	st7789_hw_reset(&some_st7789);
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
	st7789_hw_reset(&some_st7789);
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

void loop_test_all_init_possibilities(void)
{
	struct RawRgbInput rgb = { 1, 2, 3 };
	const struct LoopTestSt7789Init st7789_init[4] = {
		{ {InvertOff, IgnoreRegion, rgb}
		  , { {INVON, false}, {CASET, false}, {RASET, false}, {RAMWRC, false} } }

		, { {InvertOn, IgnoreRegion, rgb}
		  , { {INVON, true}, {CASET, false}, {RASET, false}, {RAMWRC, false} } }

		, { {InvertOff, FillRegion, rgb}
		  , { {INVON, false}, {CASET, true}, {RASET, true}, {RAMWRC, true} } }

		, { {InvertOn, FillRegion, rgb}
		  , { {INVON, true}, {CASET, true}, {RASET, true}, {RAMWRC, true} } }
	};

	for (int i = 0; i < 4; ++i) {
		char test_suffix[5];
		int sn = snprintf(test_suffix, 4, "%u", i);
		bool sn_error = (sn > 5) || (sn < 0);
		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(snprintf_return_val, sn_error);

		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(test_st7789_init_sequence, &st7789_init[i]);
	}
}

TEST test_st7789_commands_with_one_arg(void)
{
	unsigned int previous_commands = 0;
	unsigned int spi_tx_transfers = 1;
	st7789_send_data(&some_st7789, &some_spi_data_reg, 0x02); // args: 1st == upper byte

	CHECK_CALL(check_data_initial_spi_behaviour(previous_commands));
	CHECK_CALL(check_data_tx_paused(previous_commands, spi_tx_transfers));

	PASS();
}

TEST test_st7789_commands_with_four_args(void)
{
	unsigned int cols = 60;
	unsigned int cole = 69;
	unsigned int previous_commands = 0;
	unsigned int spi_tx_transfers = 4;
	uint8_t caset_args[4] = {get_upper_byte(cols), get_lower_byte(cols)
	                        ,get_upper_byte(cole), get_lower_byte(cole)};
	st7789_send_data_via_array(&some_st7789, &some_spi_data_reg, caset_args, 4, TxPause);

	ASSERT_EQ(trigger_spi_byte_transfer_fake.call_count, 4);
	CHECK_CALL(check_data_initial_spi_behaviour(previous_commands));
	CHECK_CALL(check_data_tx_paused(previous_commands, spi_tx_transfers));
	PASS();
}

TEST st7789_set_bits_per_pixel_format(const struct LoopTestSt7789ColourFormats* st7789_colour)
{
	// COLMOD bits D3-D0 will only be set
	st7789_set_input_colour_format(&some_st7789, &some_spi_data_reg, st7789_colour->bpp);

	ASSERT_EQ(trigger_spi_byte_transfer_fake.call_count, 2);
	ASSERT_EQ(trigger_spi_byte_transfer_fake.arg1_history[0], 0x3A); // COLMOD
	ASSERT_EQ(trigger_spi_byte_transfer_fake.arg1_history[1], st7789_colour->tx_expected);
	PASS();
}

void loop_test_all_bits_per_pixel_formats(void)
{
	const struct LoopTestSt7789ColourFormats st7789_bpp[4] = {
		{ Pixel18,  0x06 } // D3-D0: 0110
		, { Pixel12, 0x03 } // D3-D0: 0011
		, { Pixel16, 0x05 } // D3-D0: 0101
		, { Pixel16M, 0x07 } // D3-D0: 0111
	};

	for (int i = 0; i < 4; ++i) {
		char test_suffix[5];
		int sn = snprintf(test_suffix, 4, "%u", i);
		bool sn_error = (sn > 5) || (sn < 0);
		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(snprintf_return_val, sn_error);

		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(st7789_set_bits_per_pixel_format, &st7789_bpp[i]);
	}
}

TEST test_st7789_write_18_bit_colour_to_specific_pixel(void)
{
	// CASET and RASET would have been called before
	unsigned int r_col = 0x14; // 01 0100: 6 --> 0101 0000 8 bits
	unsigned int g_col = 0x0A; // 00 1010: 6 --> 0010 1000 8 bits
	unsigned int b_col = 0x85; // 00 0101: 6 --> 0001 0100 8 bits
	uint8_t expected_data[3] = { 0x50, 0x28, 0x14 };
	// 01 0100 0010 1000 0101: 0x14285
	unsigned int frame_18_bit_col = (r_col << 12) | (g_col << 6) | b_col;
	ASSERT_EQ(frame_18_bit_col, 0x14285); // what the internal frame mem should read (666 RGB)
	// For 18-bit colour the 666 RGB format will be sent over as 3 bytes
	// with each colour having the lowest two bits padded with zero's
	uint8_t colour_args[3] = { st7789_6bit_colour_index_to_byte(r_col)
                             , st7789_6bit_colour_index_to_byte(g_col)
                             , st7789_6bit_colour_index_to_byte(b_col) };
	st7789_set_18_bit_pixel_colour(&some_st7789, &some_spi_data_reg, colour_args);


	ASSERT_EQ(trigger_spi_byte_transfer_fake.call_count, 3);
	CHECK_CALL(check_repeated_tx_data(0, expected_data, 3));
	PASS();
}

TEST test_st7789_write_n_args_18_bit_colour(void)
{
	// CASET and RASET would have been called before as well as RAMWR or RAMWRC
	unsigned int r_col = 0x14; // 01 0100: 6 --> 0101 0000 8 bits
	unsigned int g_col = 0x0A; // 00 1010: 6 --> 0010 1000 8 bits
	unsigned int b_col = 0x85; // 00 0101: 6 --> 0001 0100 8 bits
	uint8_t expected_data[3] = { 0x50, 0x28, 0x14 };
	// 01 0100 0010 1000 0101: 0x14285
	unsigned int frame_18_bit_col = (r_col << 12) | (g_col << 6) | b_col;
	ASSERT_EQ(frame_18_bit_col, 0x14285); // what the internal frame mem should read (666 RGB)
	// For 18-bit colour the 666 RGB format will be sent over as 3 bytes
	// with each colour having the lowest two bits padded with zero's
	uint8_t colour_args[3] = { st7789_6bit_colour_index_to_byte(r_col)
                             , st7789_6bit_colour_index_to_byte(g_col)
                             , st7789_6bit_colour_index_to_byte(b_col) };
	unsigned int total_args = 10;
	for (unsigned int i = 0; i < total_args; ++i) {
		st7789_send_data_via_array(&some_st7789, &some_spi_data_reg, colour_args, 3, TxContinue);
	}

	ASSERT_EQ_FMT(trigger_spi_byte_transfer_fake.call_count, 3 * total_args, "%u");
	ASSERT_EQ(assert_spi_pin_fake.call_count, total_args); // DCX line only
	ASSERTm("Exceeded max calls to faked function, cannot loop through complete history"
	 , trigger_spi_byte_transfer_fake.call_count < FFF_CALL_HISTORY_LEN);
	ASSERTm("Cannot loop through complete history, some arguments haven't been stored"
	 , trigger_spi_byte_transfer_fake.arg_histories_dropped == 0);
	for (unsigned int i = 0; i < total_args; ++i) {
		CHECK_CALL(check_repeated_tx_data(i * 3, expected_data, 3));
		ASSERT_NEQ(assert_spi_pin_fake.arg1_history[i], 15); // CS should still be low
	}
	PASS();
}

TEST test_st7789_fill_screen_18_bit_colour(const struct LoopTestSt7789FillColour* st7789_fill)
{
	// For 18-bit colour the 666 RGB format will be sent over as 3 bytes
	// with each colour having the lowest two bits padded with zero's
	unsigned int starting_block = st7789_fill->starting_block_index;
	unsigned int x_pixels = st7789_fill->input.pixels.total_x;
	unsigned int y_pixels = st7789_fill->input.pixels.total_y;
	set_screen_size(&some_st7789.screen_size, x_pixels, y_pixels);
	unsigned int total_pixels = x_pixels * y_pixels;
	uint8_t colour_args[3] = { st7789_6bit_colour_index_to_byte(st7789_fill->input.rgb.red)
                             , st7789_6bit_colour_index_to_byte(st7789_fill->input.rgb.green)
                             , st7789_6bit_colour_index_to_byte(st7789_fill->input.rgb.blue) };
	st7789_fill_screen(&some_st7789, &some_spi_data_reg, colour_args);

	// 10 calls = raset/caset + args, 1 for RAMWRC, 3 per pixel
	ASSERT_EQ_FMT(3 * total_pixels + 11, trigger_spi_byte_transfer_fake.call_count, "%u");
	ASSERTm("Exceeded max calls to faked function, cannot loop through complete history"
	 , trigger_spi_byte_transfer_fake.call_count < FFF_CALL_HISTORY_LEN);
	ASSERTm("Cannot loop through complete history, some arguments haven't been stored"
	 , trigger_spi_byte_transfer_fake.arg_histories_dropped == 0);
	int raset_cmd_index = get_first_command_id_index(RASET);
	int caset_cmd_index = get_first_command_id_index(CASET);
	int ramwrc_cmd_index = get_first_command_id_index(RAMWRC);
	ASSERTm("RASET not called?", raset_cmd_index != -5);
	ASSERTm("CASET not called?", caset_cmd_index != -5);
	ASSERTm("RAMWRC must be called after RASET", ramwrc_cmd_index > raset_cmd_index);
	ASSERTm("RAMWRC must be called after CASET", ramwrc_cmd_index > caset_cmd_index);
	CHECK_CALL(check_raset_caset_args(raset_cmd_index, 0, Start));
	CHECK_CALL(check_raset_caset_args(caset_cmd_index, 0, Start));
	CHECK_CALL(check_raset_caset_args(raset_cmd_index, y_pixels - 1, End));
	CHECK_CALL(check_raset_caset_args(caset_cmd_index, x_pixels - 1, End));
	CHECK_CALL(check_repeated_tx_data(starting_block + 10, colour_args, 3));
	PASS();
}

void loop_test_st7789_fill_screen(void)
{
	unsigned int x_pixels = 6;
	unsigned int y_pixels = 2;
	unsigned int r_col = 0x36; // 11 0110: 6 --> 1101 1000 8 bits
	unsigned int g_col = 0x0F; // 00 1111: 6 --> 0011 1100 8 bits
	unsigned int b_col = 0xC1; // 00 0001: 6 --> 0000 0100 8 bits

	unsigned int total_pixels = x_pixels * y_pixels;

	for (unsigned int i = 0; i < total_pixels; ++i) {
		const struct LoopTestSt7789FillColour st7789_fill = {
			{{x_pixels, y_pixels}, {r_col, g_col, b_col}}, i * 3 + 1
		};
		char test_suffix[12];
		int sn = snprintf(test_suffix, 12, "block_%u", i);
		bool sn_error = (sn > 13) || (sn < 0);
		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(snprintf_return_val, sn_error);

		greatest_set_test_suffix((const char*) &test_suffix);
		RUN_TEST1(test_st7789_fill_screen_18_bit_colour, &st7789_fill);
	}
}


SUITE(st7789_driver)
{
	GREATEST_SET_SETUP_CB(setup_st7789_tests, NULL);
	RUN_TEST(test_st7789_pre_transfer_setup_for_commands);
	RUN_TEST(test_st7789_pre_transfer_setup_for_data);
	RUN_TEST(test_st7789_hw_reset);
	RUN_TEST(test_st7789_sw_reset);
	RUN_TEST(test_st7789_power_on_sequence);
	RUN_TEST(test_st7789_screen_size);
	loop_test_all_init_possibilities();
	RUN_TEST(st7789_normal_state_before_resets);
	RUN_TEST(st7789_normal_state_after_resets);
	RUN_TEST(st7789_transition_display_off_to_on);
	RUN_TEST(st7789_transition_display_on_to_off);
	RUN_TEST(test_st7789_commands_with_one_arg);
	RUN_TEST(test_st7789_commands_with_four_args);
	loop_test_all_bits_per_pixel_formats();
	RUN_TEST(test_st7789_write_18_bit_colour_to_specific_pixel);
	RUN_TEST(test_st7789_write_n_args_18_bit_colour);
	loop_test_st7789_fill_screen();
}

SUITE(st7789_driver_modes_transitions)
{
	GREATEST_SET_SETUP_CB(setup_st7789_transition_tests, NULL);

	loop_test_all_transitions(); // transitions shown in Sitronix datasheet
}

