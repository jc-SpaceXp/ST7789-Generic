#include "spi.h"

#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"

void timer_setup(unsigned int hclk_clock_divider)
{
	// Clock division = 1/(PSC + 1) as counting starts from 0
	__TIM2_CLK_ENABLE();
	TIM2->CR1 |= TIM_CR1_DIR; // Down counter
	TIM2->CNT = 0;
	TIM2->PSC = hclk_clock_divider - 1; // prescalers should be powers of 2
	TIM2->ARR = 0;
}

// us delay needed for HW reset of ST7789 display
void stm32_delay_us(unsigned int us_count)
{
	unsigned int delay_count = 15; // for a 1 us delay (minimum)
	delay_count = 15 * us_count;

	TIM2->CNT = delay_count;
	TIM2->ARR = delay_count;

	// Enable timer
	TIM2->CR1 |= TIM_CR1_CEN;

	while (!(TIM2->SR & TIM_SR_UIF)) {
		// delay
	}

	TIM2->SR &= ~TIM_SR_UIF;

	// Disable timer
	TIM2->CR1 &= ~TIM_CR1_CEN;
}

void stm32_delay_ms(unsigned int ms_count)
{
	stm32_delay_us(ms_count * 1000);
}
