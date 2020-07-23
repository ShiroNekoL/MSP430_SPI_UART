#include "gpio.h"

void GPIO_SetOutput(unsigned int GPIO_PortNum, uint8_t PINx) {
    if (GPIO_PortNum == 1) {
        P1DIR |= PINx;
        P1OUT &= ~PINx;
    } else if (GPIO_PortNum == 2) {
        P2DIR |= PINx;
        P2OUT &= ~PINx;
    }
}

void GPIO_SetInput(unsigned int GPIO_PortNum, uint8_t PINx, PullState GPIO_PullState) {
    if (GPIO_PortNum == 1) {
        P1DIR &= ~PINx;
        P1REN |= PINx;
        if (GPIO_PullState == PULL_HIGH)
            P1OUT |= PINx;
        else P1OUT &= ~PINx;
    } else if (GPIO_PortNum == 2) {
        P2DIR &= ~PINx;
        P2REN |= PINx;
        if (GPIO_PullState == PULL_HIGH)
            P2OUT |= PINx;
        else P2OUT &= ~PINx;
    }
}

void GPIO_Init() {
//    // Alternate function for UART
//    P1SEL |= SPI_MISO | SPI_MOSI;
//    P1SEL2 |= SPI_MISO | SPI_MOSI;
//    P1OUT &= ~(SPI_MISO | SPI_MOSI);

    GPIO_SetOutput(1, LED1);
//    GPIO_SetOutput(2, VAL_EN | VAL_IN1 | VAL_IN2);
//    GPIO_SetInput(1, AUX, PULL_HIGH);

    // Enable interrupt for AUX pin
    //P1IE |= AUX;
    //P1IES |= AUX;
    //P1IFG &= ~AUX;
}
