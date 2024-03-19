#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

struct St7789Internals {
	uint32_t* res_addr;
	unsigned int res_pin;
};

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int));


#endif /* ST7789_H */
