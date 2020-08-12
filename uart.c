#include <msp430.h>
#include "uart.h"
#include "mcu.h"

void UART_Init() {
    P1SEL  |=  BIT1 | BIT2;             // P1.1 UCA0RXD input
    P1SEL2 |=  BIT1 | BIT2;             // P1.2 UCA0TXD output

    UCA0CTL1 |=  UCSSEL_2 | UCSWRST;    // USCI Clock = SMCLK,USCI_A0 disabled

    UCA0BR0 = 0x82;                     // 0x0682 From datasheet table
    UCA0BR1 = 0x06;                     // Selects baudrate = 9600bps, Clk = SMCLK

    UCA0MCTL = UCBRS_1;                 // Modulation value = 1 from datasheet

    UCA0STAT |= UCLISTEN;               // Loop back mode enabled

    UCA0CTL1 &= ~UCSWRST;               // Clear UCSWRST to enable USCI_A0
}

void UART_WriteStr(char* str) {
    int i;
    for(i = 0; str[i] != '\0'; i++) {
        while (!(IFG2 & UCA0TXIFG));    // TX buffer ready?
        UCA0TXBUF = str[i];
    }
}

void UART_WriteNChar(char* data, int n) {
    while(n--) {
        while (!(IFG2 & UCA0TXIFG));
        UCA0TXBUF = *data++;
    }
}

void UART_WriteChar(char character) {
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = character;
}

void UART_PrintDec(int32_t number) {
    char buffer[8 + 1];
    int32_t abs_num = abs(number);

    uint8_t size = 0;
    while (abs_num) {
        buffer[size++] = '0' + abs_num % 10;
        abs_num /= 10;
    }

    if (size == 0)
        buffer[size++] = '0';
    if ((int) number < 0)
        buffer[size++] = '-';

    uint8_t i, j;
    for (i = 0, j = size - 1; i < j; i++, j--) {
        char temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    buffer[size] = '\0';

    UART_WriteStr(buffer);
}

void UART_PrintHex8(uint8_t number) {
    char buf[2 + 1];
    char *str = &buf[3 - 1];

    *str = '\0';

    uint8_t base = 16;

    do {
        uint8_t temp = number;
        number /= base;
        char char_temp = temp - base * number;
        *--str = char_temp < 10 ? char_temp + '0' : char_temp + 'A' - 10;
    } while (number);

    UART_WriteStr(str);
}

void UART_PrintHex32(uint32_t number) {
    char buf[8 + 1];
    char *str = &buf[9 - 1];

    *str = '\0';

    uint32_t base = 16;

    do {
        uint32_t temp = number;
        number /= base;
        char char_temp = temp - base * number;
        *--str = char_temp < 10 ? char_temp + '0' : char_temp + 'A' - 10;
    } while (number);

    UART_WriteStr(str);
}
