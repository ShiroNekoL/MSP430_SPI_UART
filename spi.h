#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>
#include "gpio.h"

void SPI_Init();
void SPI_SendByte(uint8_t byte);

#endif /* SPI_H_ */
