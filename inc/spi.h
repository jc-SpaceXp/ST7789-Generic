#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void assert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin);
void deassert_spi_pin(uint32_t* gpio_output_addr, unsigned int gpio_pin);

void setup_hw_spi(void);

#endif /* SPI_H */
