#ifndef ST7789_PRIVATE_H
#define ST7789_PRIVATE_H

struct St7789SpiPin {
	volatile uint32_t* assert_addr;
	volatile uint32_t* deassert_addr;
	unsigned int pin;
};

struct St7789Internals {
	struct St7789SpiPin rsx; // RS, reset
	struct St7789SpiPin csx; // CS, chip select
	struct St7789SpiPin dcx; // DC/X, data or command (DC/X = 0 for commands)
};

#endif /* ST7789_PRIVATE_H */
