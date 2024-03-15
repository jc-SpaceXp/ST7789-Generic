#include "stm32g4_main.h"
#include "gpio_g431.h"

#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"

int main (void)
{
	HAL_Init();

	gpio_setup();

	for (;;) {
		HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
		HAL_Delay(100);
	}

	return 0;
}
