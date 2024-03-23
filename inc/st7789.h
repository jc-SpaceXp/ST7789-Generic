#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stdbool.h>

struct St7789SpiPin;
struct St7789Modes;
struct St7789Internals;
enum SpiSignal { RSX, CSX, DCX };
enum SleepModes { SleepIn, SleepOut };
enum DisplayModes { NormalDisp, PartialDisp };

void set_spi_pin_details(struct St7789SpiPin* st7789_pin
                        , volatile uint32_t* assert_addr
                        , volatile uint32_t* deassert_addr
                        , unsigned int pin);
void set_st7789_pin_details(struct St7789Internals* st7789_driver, struct St7789SpiPin* st7789_pin
                           , enum SpiSignal spi_signal);

void initial_st7789_modes(struct St7789Modes* current_st7789_mode);
enum SleepModes get_current_sleep_mode(struct St7789Modes current_st7789_mode);
enum DisplayModes get_current_display_mode(struct St7789Modes current_st7789_mode);
bool get_current_idle_mode(struct St7789Modes current_st7789_mode);
bool display_is_on(struct St7789Modes current_st7789_mode);

void st7789_hw_reset(struct St7789Internals* st7789_driver, void (*delay_us)(unsigned int));
void st7789_sw_reset(struct St7789Internals* st7789_driver, uint16_t* spi_tx_reg);
void st7789_sleep_out_command(struct St7789Internals* st7789_driver, uint16_t* spi_tx_reg);


#endif /* ST7789_H */
