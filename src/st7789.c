#include "st7789.h"
#include "st7789_private.h" // for opaque pointer
#include "spi.h"


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

void init_st7789_callbacks(struct UserCallbacksSt7789* dest, const struct UserCallbacksSt7789* src)
{
	*dest = *src;
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

void set_screen_size(struct St7789Size* screen_size, unsigned int x, unsigned int y)
{
	screen_size->x = x;
	screen_size->y = y;
}

void st7789_set_input_colour_format(struct St7789Internals* st7789_driver
                                   , volatile uint32_t* spi_tx_reg
                                   , enum BitsPerPixel bpp)
{
	// input RGB is converted in st7889 via a LUT to display an output colour
	// typically the LUT is for the 18-bit output (18 on reset for both input/output)
	// input is refered to as the colour interface in the datasheet
	uint8_t data = 0;
	if ((bpp == Pixel24) || (bpp == Pixel16M)) {
		data |= 0x07;
	} else if (bpp == Pixel18) {
		data |= 0x06;
	} else if (bpp == Pixel16) {
		data |= 0x05;
	} else if (bpp == Pixel12) {
		data |= 0x03;
	}

	st7789_send_command(st7789_driver, spi_tx_reg, COLMOD);
	st7789_send_data(st7789_driver, spi_tx_reg, data);

	st7789_driver->pixel_depth = bpp;
}

void st7789_hw_reset(struct St7789Internals* st7789_driver)
{
	// Must be a hi-lo transition, pulse RES for 10us minimum
	// Ignored in sleep-in mode
	assert_spi_pin(st7789_driver->rsx.assert_addr, st7789_driver->rsx.pin);
	st7789_driver->user_defined.delay_us(15);
	deassert_spi_pin(st7789_driver->rsx.deassert_addr, st7789_driver->rsx.pin);
	st7789_driver->user_defined.delay_us(10); // 5-9 us for a valid reset
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

void pre_st7789_transfer(const struct St7789Internals* st7789_driver, enum TxCmdOrData data)
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

static void st7789_transfer_byte(volatile uint32_t* spi_tx_reg, uint8_t tx_byte
                                , const struct St7789Internals* st7789_driver)
{
	while (!st7789_driver->user_defined.tx_ready_to_transmit()) {
		// Wait on SPI to become free
	}
	trigger_spi_byte_transfer(spi_tx_reg, tx_byte);
}

static void post_st7789_transfer(const struct St7789Internals* st7789_driver
                                , enum TxContinueOrPause post_tx_action)
{
	while (!st7789_driver->user_defined.tx_complete()) {
		// Wait on SPI transmission
	}

	if (post_tx_action == TxPause) {
		pause_transmission(st7789_driver);
	}

}

// Assumes no args for now
void st7789_send_command(struct St7789Internals* st7789_driver
                        , volatile uint32_t* spi_tx_reg
                        , uint8_t command_id)
{
	pre_st7789_transfer(st7789_driver, TxCmd);
	st7789_transfer_byte(spi_tx_reg, command_id, st7789_driver);
	post_st7789_transfer(st7789_driver, TxPause);
	update_st7789_modes(&st7789_driver->st7789_mode, command_id);
}

void st7789_send_data(const struct St7789Internals* st7789_driver
                     , volatile uint32_t* spi_tx_reg
                     , uint8_t data)
{
	pre_st7789_transfer(st7789_driver, TxData);
	st7789_transfer_byte(spi_tx_reg, data, st7789_driver);
	post_st7789_transfer(st7789_driver, TxPause);
}

void st7789_send_data_via_array(const struct St7789Internals* st7789_driver
                               , volatile uint32_t* spi_tx_reg
                               , uint8_t* data
                               , size_t total_args
                               , enum TxContinueOrPause post_tx_action)
{
	pre_st7789_transfer(st7789_driver, TxData);

	for (size_t i = 0; i < total_args; ++i) {
		st7789_transfer_byte(spi_tx_reg, *(data + i), st7789_driver);
	}

	post_st7789_transfer(st7789_driver, post_tx_action);
}

void st7789_power_on_sequence(struct St7789Internals* st7789_driver
                             , volatile uint32_t* spi_tx_reg)
{
	void delay_ms(unsigned int ms_delay) {
		st7789_driver->user_defined.delay_us(1000 * ms_delay);
	}
	st7789_hw_reset(st7789_driver);
	delay_ms(120);
	st7789_send_command(st7789_driver, spi_tx_reg, SWRESET);
	delay_ms(120);
}

void st7789_init_sequence(struct St7789Internals* st7789_driver
                         , volatile uint32_t* spi_tx_reg
                         , enum InitInversion invert
                         , enum FillScreenRegion screen_region
                         , struct St7789Size init_size
                         , struct RawRgbInput rgb
                         , enum BitsPerPixel bpp)
{
	void delay_ms(unsigned int ms_delay) {
		st7789_driver->user_defined.delay_us(1000 * ms_delay);
	}
	set_screen_size(&st7789_driver->screen_size, init_size.x, init_size.y);
	st7789_driver->pixel_depth = Pixel18; // 18-bits per pixel on reset
	// Initial modes are:
	// SPLIN, DISPOFF, NORMAL MODE, IDLE OFF
	st7789_power_on_sequence(st7789_driver, spi_tx_reg);
	st7789_send_command(st7789_driver, spi_tx_reg, SLPOUT);
	delay_ms(120);
	// Optionally invert screen, necessary for some screens to display correct colour
	if (invert == InvertOn) { st7789_send_command(st7789_driver, spi_tx_reg, INVON); }
	if (bpp != Pixel18) {
		st7789_set_input_colour_format(st7789_driver, spi_tx_reg, bpp);
	}
	// Set screen size and set a colour
	if (screen_region == FillRegion) {
		st7789_fill_screen(st7789_driver, spi_tx_reg, rgb, bpp);
	}

	st7789_send_command(st7789_driver, spi_tx_reg, DISPON);
}

static void st7789_set_x_or_y_region(struct St7789Internals* st7789_driver
                                    , volatile uint32_t* spi_tx_reg
                                    , enum TxCasetOrRaset cmd
                                    , unsigned int xy_start
                                    , unsigned int xy_end)
{
	uint8_t cmd_args[4] = { get_upper_byte(xy_start), get_lower_byte(xy_start)
	                      , get_upper_byte(xy_end),   get_lower_byte(xy_end) };

	uint8_t command_id = CASET;
	if (cmd == (TxRaset || TxYpos)) {
		command_id = RASET;
	}

	st7789_send_command(st7789_driver, spi_tx_reg, command_id);
	st7789_send_data_via_array(st7789_driver, spi_tx_reg, cmd_args, 4, TxPause);
}

void st7789_set_x_coordinates(struct St7789Internals* st7789_driver
                             , volatile uint32_t* spi_tx_reg
                             , unsigned int x_start
                             , unsigned int x_end)
{
	st7789_set_x_or_y_region(st7789_driver, spi_tx_reg, TxCaset, x_start, x_end);
}

void st7789_set_y_coordinates(struct St7789Internals* st7789_driver
                             , volatile uint32_t* spi_tx_reg
                             , unsigned int y_start
                             , unsigned int y_end)
{
	st7789_set_x_or_y_region(st7789_driver, spi_tx_reg, TxRaset, y_start, y_end);
}

union RgbInputFormat rgb_to_st7789_formatter(struct RawRgbInput rgb, enum BitsPerPixel bpp)
{
	union RgbInputFormat rgb_st7789;
	rgb_st7789.rgb666.total_bytes = 2;
	if ((bpp == Pixel24) || (bpp == Pixel16M)) {
		rgb_st7789.rgb888.bytes[0] = (uint8_t) rgb.red;
		rgb_st7789.rgb888.bytes[1] = (uint8_t) rgb.green;
		rgb_st7789.rgb888.bytes[2] = (uint8_t) rgb.blue;
		rgb_st7789.rgb888.total_bytes = 3;
	} else if (bpp == Pixel18) {
		rgb_st7789.rgb666.bytes[0] = st7789_6bit_colour_index_to_byte(rgb.red);
		rgb_st7789.rgb666.bytes[1] = st7789_6bit_colour_index_to_byte(rgb.green);
		rgb_st7789.rgb666.bytes[2] = st7789_6bit_colour_index_to_byte(rgb.blue);
		rgb_st7789.rgb666.total_bytes = 3;
	} else if (bpp == Pixel16) {
		// Format: RRRR RGGG GGGB BBBB (keep lowest 5/6 bits for each channel)
		rgb_st7789.rgb565.bytes[0] = ((rgb.red & 0x1F) << 3)   | ((rgb.green & 0x38) >> 3);
		rgb_st7789.rgb565.bytes[1] = ((rgb.green & 0x07) << 5) | (rgb.blue & 0x1F);
	} else if (bpp == Pixel12) {
		// Format: RRRR GGGG BBBB xxxx (keep lowest 4 bits for each channel)
		rgb_st7789.rgb444.bytes[0] = ((rgb.red & 0x0F) << 4)  | (rgb.green & 0x0F);
		rgb_st7789.rgb444.bytes[1] = (rgb.blue & 0x0F) << 4;
	}

	return rgb_st7789;
}

void st7789_set_pixel_colour(struct St7789Internals* st7789_driver
                            , volatile uint32_t* spi_tx_reg
                            , struct RawRgbInput rgb_input
                            , enum BitsPerPixel bpp)
{
	union RgbInputFormat rgb_format = rgb_to_st7789_formatter(rgb_input, bpp);

	// Can refer to any struct as memory locations for these pointers will be the same
	uint8_t* args = &rgb_format.rgb888.bytes[0];
	unsigned int total_bytes = rgb_format.rgb888.total_bytes;
	st7789_send_data_via_array(st7789_driver, spi_tx_reg, args, total_bytes, TxContinue);
}

void st7789_fill_screen(struct St7789Internals* st7789_driver
                       , volatile uint32_t* spi_tx_reg
                       , struct RawRgbInput rgb_input
                       , enum BitsPerPixel bpp)
{
	// set_screen_size() must be called before
	// RASET/CASET require a -1 as they are zero indexed
	// e.g. 240 pixels would be 0-239
	unsigned int y_start = 0;
	unsigned int y_end = st7789_driver->screen_size.y;
	st7789_set_y_coordinates(st7789_driver, spi_tx_reg, y_start, y_end - 1);
	unsigned int x_start = 0;
	unsigned int x_end = st7789_driver->screen_size.x;
	st7789_set_x_coordinates(st7789_driver, spi_tx_reg, x_start, x_end - 1);

	union RgbInputFormat rgb_format = rgb_to_st7789_formatter(rgb_input, bpp);
	uint8_t* args = &rgb_format.rgb888.bytes[0];
	unsigned int total_bytes = rgb_format.rgb888.total_bytes;

	st7789_send_command(st7789_driver, spi_tx_reg, RAMWR);
	for (int y = 0; y < (int) y_end; ++y) {
		for (int x = 0; x < (int) x_end; ++x) {
			st7789_send_data_via_array(st7789_driver, spi_tx_reg, args, total_bytes, TxContinue);
		}
	}
}

void st7789_set_region(struct St7789Internals* st7789_driver
                      , volatile uint32_t* spi_tx_reg
                      , struct RegionInput region)
{
	st7789_set_x_coordinates(st7789_driver, spi_tx_reg, region.x.start, region.x.end - 1);
	st7789_set_y_coordinates(st7789_driver, spi_tx_reg, region.y.start, region.y.end - 1);
}

void st7789_fill_region(struct St7789Internals* st7789_driver
                       , volatile uint32_t* spi_tx_reg
                       , struct RegionInput region
                       , struct RawRgbInput rgb_input
                       , enum BitsPerPixel bpp)
{

	st7789_set_region(st7789_driver, spi_tx_reg, region);

	union RgbInputFormat rgb_format = rgb_to_st7789_formatter(rgb_input, bpp);
	uint8_t* args = &rgb_format.rgb888.bytes[0];
	unsigned int total_bytes = rgb_format.rgb888.total_bytes;

	st7789_send_command(st7789_driver, spi_tx_reg, RAMWR);
	for (int y = region.y.start; y < (int) region.y.end; ++y) {
		for (int x = region.x.start; x < (int) region.x.end; ++x) {
			st7789_send_data_via_array(st7789_driver, spi_tx_reg, args, total_bytes, TxContinue);
		}
	}
	// N arg commands are terminated once another command is sent
	st7789_send_command(st7789_driver, spi_tx_reg, NOP);
}

void st7789_putchar(struct St7789Internals* st7789_driver
                   , volatile uint32_t* spi_tx_reg
                   , const struct FontArguments* font
                   , enum BitsPerPixel bpp)
{
	// Assume font is 5x7 for now
	// glcdfonts array is stored column wise, where MSB = bottom, LSB = top
	struct RegionInput region = { {font->x_start, font->x_start + 5}
	                            , {font->y_start, font->y_start + 7} };
	st7789_set_region(st7789_driver, spi_tx_reg, region);

	union RgbInputFormat rgb_format = rgb_to_st7789_formatter(font->rgb_background, bpp);
	uint8_t* args = &rgb_format.rgb888.bytes[0];
	unsigned int total_bytes = rgb_format.rgb888.total_bytes;

	unsigned int font_offset = font->output_char * 5;
	unsigned int x_offset = 0;
	unsigned int y_mask = 0;

	st7789_send_command(st7789_driver, spi_tx_reg, RAMWR);
	for (int y = region.y.start; y < (int) region.y.end; ++y) {
		for (int x = region.x.start; x < (int) region.x.end; ++x) {
			x_offset = x - region.x.start;
			y_mask = 1 << (y - region.y.start);
			if (font->bitmap[font_offset + x_offset] & y_mask) {
				rgb_format = rgb_to_st7789_formatter(font->rgb_background, bpp);
			} else {
				rgb_format = rgb_to_st7789_formatter(font->rgb_foreground, bpp);
			}
			st7789_send_data_via_array(st7789_driver, spi_tx_reg, args, total_bytes, TxContinue);
		}
	}
	// N arg commands are terminated once another command is sent
	st7789_send_command(st7789_driver, spi_tx_reg, NOP);
}
