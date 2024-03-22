#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

struct St7789SpiPin;
struct St7789Internals;
enum SpiSignal { RSX, CSX, DCX };

void set_spi_pin_details(struct St7789SpiPin* st7789_pin
                        , volatile uint32_t* assert_addr
                        , volatile uint32_t* deassert_addr
                        , unsigned int pin);
void set_st7789_pin_details(struct St7789Internals* st7789_driver, struct St7789SpiPin* st7789_pin
                           , enum SpiSignal spi_signal);

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int));
void st7789_sw_reset(struct St7789Internals* st7789_driver, uint16_t* spi_tx_reg);


#endif /* ST7789_H */
