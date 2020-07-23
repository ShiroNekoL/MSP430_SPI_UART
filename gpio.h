#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include <msp430.h>

// Port 1
#define LED1        BIT0
//#define      BIT1
//#define      BIT2
//#define       BIT3
//#define       BIT4
#define SPI_CLK     BIT5
#define SPI_MISO    BIT6
#define SPI_MOSI    BIT7

// Port 2
//#define    BIT0
//#define   BIT1
//#define  BIT2
//#define  BIT3
//#define   BIT5

typedef enum {
    PULL_LOW = 0,
    PULL_HIGH = !PULL_LOW
} PullState;

void GPIO_SetOutput(unsigned int port_num, uint8_t PINx);
void GPIO_SetInput(unsigned int port_num, uint8_t PINx, PullState GPIO_PullState);
void GPIO_Init();

#endif /* GPIO_H_ */
