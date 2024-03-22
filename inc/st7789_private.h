#ifndef ST7789_PRIVATE_H
#define ST7789_PRIVATE_H

struct St7789Internals {
	volatile uint32_t* res_addr; // RS
	unsigned int res_pin;
};

#endif /* ST7789_PRIVATE_H */
