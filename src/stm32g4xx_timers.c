#include "spi.h"

#include "stm32g4xx.h"
#include "stm32g4xx_ll_spi.h"

void timer_setup(void)
{
	__TIM2_CLK_ENABLE();
	TIM2->CR1 |= TIM_CR1_DIR; // Down counter
	TIM2->CNT = 0;
	TIM2->PSC = 0;
	TIM2->ARR = 0;
}

// 5-9 us for RST delay (HW reset)
// Display is then blanked for 120ms
void stm32_delay_us(unsigned int hclk_clock_divider)
{
	unsigned int delay_count = 15; // for a 1 us delay (minimum)
	delay_count = 15 * hclk_clock_divider; // for a 1 us delay (minimum)

	TIM2->CNT = delay_count;
	TIM2->PSC = 0;
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
