#include "spi.h"

uint8_t rx_buffer;
uint8_t tx_buffer = 'a';

void SPI_Init() {
    // Alternate function for SPI
    P1SEL |= SPI_MISO | SPI_MOSI | SPI_CLK;
    P1SEL2 |= SPI_MISO | SPI_MOSI | SPI_CLK;
    P1OUT &= ~(SPI_MISO | SPI_MOSI);

    // Config SPI
    UCB0CTL0 |= UCCKPL | UCMSB | UCMST | UCSYNC;    // Data change on UCLK edge, capture at the following edge
                                                    // The inactive state is high
                                                    // Reference timing diagram at page 443 of the reference manual
                                                    // MSB first / Master mode / Synchronous mode
    UCB0CTL1 |= UCSSEL_1;                           // ACLK
    UCB0CTL1 &= ~UCSWRST;                           // Initialize USCI
    UCB0BR0 |= 0x01;                                // Bit clock configuration, currently = BRCLK / 1
    UCB0BR1 |= 0;
    UC0IE |= UCB0RXIE;                              // Enable receive interrupt
    _delay_cycles(1);
}


// TX ISR
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR() {
    while (!(IFG2 & UCB0TXIFG))
        continue;
    UCB0TXBUF = tx_buffer;
    P1OUT |= LED1;
    UC0IE &= ~UCB0TXIE;
}

// RX ISR
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR() {
    while (!(IFG2 & UCB0RXIFG))
        continue;
    if (UCB0RXBUF == 'a')
        P1OUT |= LED1;
}
