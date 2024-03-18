#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

void st7789_hw_reset(uint32_t* res_addr, unsigned int res_pin, void (*delay_us)(unsigned int));


#endif /* ST7789_H */
