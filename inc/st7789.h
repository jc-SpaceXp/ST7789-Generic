#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

struct St7789Internals;

void set_res_addr(struct St7789Internals* st7789_driver, volatile uint32_t* res_addr);
void set_res_pin(struct St7789Internals* st7789_driver, unsigned int res_pin);

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int));


#endif /* ST7789_H */
