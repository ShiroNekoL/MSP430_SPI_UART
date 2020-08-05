#include "spi.h"

volatile uint8_t SPI_Buffer = 0;

void SPI_Init() {
    UCB0CTL1 = UCSWRST;
    UCB0CTL0 = UCSYNC + UCSPB + UCMSB + UCCKPH;   // UCMSB + UCMST + UCSYNC; // 3-pin, 8-bit SPI master
    UCB0CTL1 |= UCSSEL_2;                         // SMCLK
    UCB0BR0 = 0x02;                               // Frequency CPU / 2 (16Mhz / 2 = 8 Mhz SPI)
    UCB0BR1 = 0;

    P1SEL  |= SCLK | SDI | SDO;                   // P1.6 is MISO and P1.7 is MOSI
    P1SEL2 |= SCLK | SDI | SDO;

    P1DIR |= SCLK | SDO;
    P1DIR &= ~SDI;

    P2DIR |= CS;// | CS2;                         // P2.0 CS (chip select)
    P2OUT |= CS;// | CS2;

    //P1DIR &= ~(INT1 | INT2);                      // P1.4 and P1.3 as INT (INTERRUPT, not used yet)

    UCB0CTL1 &= ~UCSWRST;                         // Initialize USCI state machine
}

void SPI_TXReady() {
    while (!(IFG2 & UCB0TXIFG)); // TX buffer ready?
}

void SPI_RXReady() {
    while (!(IFG2 & UCB0RXIFG)); // RX Received?
}

void SPI_SendByte(uint8_t data) {
    SPI_TXReady();
    UCB0TXBUF = data;            // Send data over SPI to Slave
}

void SPI_Receive() {
    SPI_RXReady();
    SPI_Buffer = UCB0RXBUF;         // Store received data
}

void SPI_TransferByte(uint8_t data) {
    SPI_SendByte(data);
    SPI_Receive();
}

void SPI_ChipEnable(uint8_t ChipPort, uint8_t ChipPin) {
    if (ChipPort == Port_1)
        P1OUT &= ~ChipPin;
    else if (ChipPort == Port_2)
        P2OUT &= ~ChipPin;
}

void SPI_ChipDisable(uint8_t ChipPort, uint8_t ChipPin) {
    if (ChipPort == Port_1)
        P1OUT |= ChipPin;
    else if (ChipPort == Port_2)
        P2OUT |= ChipPin;
}
