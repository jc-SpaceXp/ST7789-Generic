#ifndef ST7789_PRIVATE_H
#define ST7789_PRIVATE_H

#include "user_defined_callbacks.h"

struct St7789SpiPin {
	volatile uint32_t* assert_addr;
	volatile uint32_t* deassert_addr;
	unsigned int pin;
};

// 5 controllable (6 total), 4 modes w/ sleep out and 1 w/ sleep in
struct St7789Modes {
	enum SleepModes sleep_mode;
	enum DisplayModes display_mode;
	bool idle_mode;
	bool display_on;
};

struct St7789Size {
	// Typically 240x240
	unsigned int x;
	unsigned int y;
};

struct St7789Internals {
	struct St7789SpiPin rsx; // RS, reset
	struct St7789SpiPin csx; // CS, chip select
	struct St7789SpiPin dcx; // DC/X, data or command (DC/X = 0 for commands)

	struct St7789Modes st7789_mode;
	struct St7789Size screen_size;

	enum BitsPerPixel pixel_depth;

	struct UserCallbacksSt7789 user_defined;
};

#endif /* ST7789_PRIVATE_H */
