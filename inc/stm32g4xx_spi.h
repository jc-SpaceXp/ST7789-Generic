#ifndef STM32G4xx_SPI_H
#define STM32G4xx_SPI_H

#include <stdbool.h>

// Don't use GPIO_PIN_x as this is the bit mask not pin number
#define SPI_CLK_PIN    3
#define SPI_CLK_PORT   GPIOB
#define SPI_MISO_PIN   4
#define SPI_MISO_PORT  GPIOB
#define SPI_MOSI_PIN   5
#define SPI_MOSI_PORT  GPIOB
#define SPI_CS_PIN     11
#define SPI_CS_PORT    GPIOA
// Add CMD too

void setup_hw_spi(void);
bool tx_complete(void);

#endif /* STM32G4xx_SPI_H */
