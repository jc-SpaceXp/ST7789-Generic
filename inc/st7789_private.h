#ifndef ST7789_PRIVATE_H
#define ST7789_PRIVATE_H

struct St7789Internals {
	volatile uint32_t* res_addr; // RS
	volatile uint32_t* cs_addr;  // CS
	volatile uint32_t* dcx_addr; // DC/X (data or command)
	unsigned int res_pin;
	unsigned int cs_pin;
	unsigned int dcx_pin;
};

#endif /* ST7789_PRIVATE_H */
