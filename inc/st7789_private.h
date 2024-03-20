#ifndef ST7789_PRIVATE_H
#define ST7789_PRIVATE_H

struct St7789Internals {
	uint32_t* res_addr;
	//uint32_t* cs_addr;
	unsigned int res_pin;
	//unsigned int cs_pin;
};

#endif /* ST7789_PRIVATE_H */
