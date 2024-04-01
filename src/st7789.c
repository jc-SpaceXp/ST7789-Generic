#include "st7789.h"
#include "st7789_private.h" // for opaque pointer
#include "spi.h"
#include "stm32g4xx_spi.h"



static void set_sleep_mode(struct St7789Modes* st7789_mode, enum SleepModes new_sleep_mode)
{
	st7789_mode->sleep_mode = new_sleep_mode;
}

static void set_display_mode(struct St7789Modes* st7789_mode, enum DisplayModes new_display_mode)
{
	st7789_mode->display_mode = new_display_mode;
}

static void set_display_on(struct St7789Modes* st7789_mode, bool new_display_on)
{
	st7789_mode->display_on = new_display_on;
}

static void set_idle_mode(struct St7789Modes* st7789_mode, bool new_idle_state)
{
	st7789_mode->idle_mode = new_idle_state;
}

uint8_t get_upper_byte(uint16_t data)
{
	return (data >> 8);
}

uint8_t get_lower_byte(uint16_t data)
{
	return (data & 0xFF);
}

void set_spi_pin_details(struct St7789SpiPin* st7789_pin
                        , volatile uint32_t* assert_addr
                        , volatile uint32_t* deassert_addr
                        , unsigned int pin)
{
	st7789_pin->assert_addr = assert_addr;
	st7789_pin->deassert_addr = deassert_addr;
	st7789_pin->pin = pin;
}

void set_st7789_pin_details(struct St7789Internals* st7789_driver, struct St7789SpiPin* st7789_pin
                           , enum SpiSignal spi_signal)
{
	struct St7789SpiPin* dest_pin = st7789_pin; // dst = source, will be overwritten below
	if (spi_signal == RSX) {
		dest_pin = &st7789_driver->rsx;
	} else if (spi_signal == CSX) {
		dest_pin = &st7789_driver->csx;
	} else if (spi_signal == DCX) {
		dest_pin = &st7789_driver->dcx;
	}

	set_spi_pin_details(dest_pin
	                   , st7789_pin->assert_addr
	                   , st7789_pin->deassert_addr
	                   , st7789_pin->pin);
}

void initial_st7789_modes(struct St7789Modes* st7789_mode)
{
	// RDDPM command in the data sheets specifies these default values
	// As does the power flow chart too
	st7789_mode->sleep_mode = SleepIn; // need to wait 120ms before sending out SleepOut
	st7789_mode->display_mode = NormalDisp;
	st7789_mode->idle_mode = false;
	st7789_mode->display_on = false;
}

static void update_st7789_modes(struct St7789Modes* st7789_mode, uint8_t command_id)
{
	if (command_id == SLPIN) {
		set_sleep_mode(st7789_mode, SleepIn);
	} else if (command_id == SLPOUT) {
		set_sleep_mode(st7789_mode, SleepOut);
	} else if (command_id == PLTON) {
		set_display_mode(st7789_mode, PartialDisp);
	} else if (command_id == NORON) {
		set_display_mode(st7789_mode, NormalDisp);
	} else if (command_id == DISPON) {
		set_display_on(st7789_mode, true);
	} else if (command_id == DISPOFF) {
		set_display_on(st7789_mode, false);
	} else if (command_id == IDLEON) {
		set_idle_mode(st7789_mode, true);
	} else if (command_id == IDLEOFF) {
		set_idle_mode(st7789_mode, false);
	}
}

enum SleepModes get_current_sleep_mode(struct St7789Modes current_st7789_mode)
{
	return current_st7789_mode.sleep_mode;
}

enum DisplayModes get_current_display_mode(struct St7789Modes current_st7789_mode)
{
	return current_st7789_mode.display_mode;
}

bool get_current_idle_mode(struct St7789Modes current_st7789_mode)
{
	return current_st7789_mode.idle_mode;
}

bool display_is_on(struct St7789Modes current_st7789_mode)
{
	return current_st7789_mode.display_on;
}

uint8_t st7789_6bit_colour_index_to_byte(unsigned int colour)
{
	unsigned int six_bit_colour = colour & 0x3F;
	return (six_bit_colour << 2);
}

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int))
{
	// Must be a hi-lo transition, pulse RES for 10us minimum
	// Ignored in sleep-in mode
	assert_spi_pin(st7789_driver->rsx.assert_addr, st7789_driver->rsx.pin);
	delay_us(15);
	deassert_spi_pin(st7789_driver->rsx.deassert_addr, st7789_driver->rsx.pin);
	delay_us(10); // 5-9 us for a valid reset
	assert_spi_pin(st7789_driver->rsx.assert_addr, st7789_driver->rsx.pin);
	// Display is then blanked for 120ms
}

// Can pause/interrupt data transmission by pulling CS high
// If we stop a byte before D0 is sent we must resend that byte
// However, we can pause transmission after D0 has been sent in these situations
// 1) Command - pause - Command
// 2) Command - pause - Parameter
// 3) Parameter - pause - Command
// 4) Parameter - pause - Parameter
static void pause_transmission(const struct St7789Internals* st7789_driver)
{
	assert_spi_pin(st7789_driver->csx.assert_addr, st7789_driver->csx.pin);
}

void pre_st7789_transfer(struct St7789Internals* st7789_driver, enum TxCmdOrData data)
{
	void (*data_or_command)(volatile uint32_t*, unsigned int) = &deassert_spi_pin;
	volatile uint32_t* write_address = st7789_driver->dcx.deassert_addr;

	if (data == TxData) {
		data_or_command = &assert_spi_pin;
		write_address = st7789_driver->dcx.assert_addr;
	}

	data_or_command(write_address, st7789_driver->dcx.pin);
	deassert_spi_pin(st7789_driver->csx.deassert_addr, st7789_driver->csx.pin);
}

// Assumes no args for now
void st7789_send_command(struct St7789Internals* st7789_driver
                        , volatile uint32_t* spi_tx_reg
                        , uint8_t command_id)
{
	// DC/X is pulled lo to indicate a CMD being sent
	deassert_spi_pin(st7789_driver->dcx.deassert_addr, st7789_driver->dcx.pin);
	deassert_spi_pin(st7789_driver->csx.deassert_addr, st7789_driver->csx.pin);

	while (!tx_ready_to_transmit()) {
		// Wait on SPI to become free
	}
	trigger_spi_byte_transfer(spi_tx_reg, command_id);

	while (!tx_complete()) {
		// Wait on SPI transmission
	}
	pause_transmission(st7789_driver);

	// Update internal state too
	update_st7789_modes(&st7789_driver->st7789_mode, command_id);
}

void st7789_send_data(const struct St7789Internals* st7789_driver
                     , volatile uint32_t* spi_tx_reg
                     , uint8_t data)
{
	// DC/X is pulled hi to indicate data being sent
	assert_spi_pin(st7789_driver->dcx.assert_addr, st7789_driver->dcx.pin);
	deassert_spi_pin(st7789_driver->csx.deassert_addr, st7789_driver->csx.pin);

	while (!tx_ready_to_transmit()) {
		// Wait on SPI to become free
	}
	trigger_spi_byte_transfer(spi_tx_reg, data);

	while (!tx_complete()) {
		// Wait on SPI transmission
	}
	pause_transmission(st7789_driver);
}

void st7789_send_data_via_array(const struct St7789Internals* st7789_driver
                               , volatile uint32_t* spi_tx_reg
                               , uint8_t* data
                               , size_t total_args)
{
	// DC/X is pulled hi to indicate data being sent
	assert_spi_pin(st7789_driver->dcx.assert_addr, st7789_driver->dcx.pin);
	deassert_spi_pin(st7789_driver->csx.deassert_addr, st7789_driver->csx.pin);

	for (size_t i = 0; i < total_args; ++i) {
		while (!tx_ready_to_transmit()) {
			// Wait on SPI to become free
		}
		trigger_spi_byte_transfer(spi_tx_reg, *(data + i));
	}

	while (!tx_complete()) {
		// Wait on SPI transmission
	}
	pause_transmission(st7789_driver);
}
