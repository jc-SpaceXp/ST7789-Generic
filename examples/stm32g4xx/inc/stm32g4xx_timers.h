#ifndef STM32G4xx_TIMERS_H
#define STM32G4xx_TIMERS_H

void timer_setup(unsigned int hclk_clock_divider);
void stm32_delay_us(unsigned int us_count);
void stm32_delay_ms(unsigned int ms_count);

#endif /* STM32G4xx_TIMERS_H */
