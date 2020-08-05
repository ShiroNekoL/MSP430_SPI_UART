#include <stdint.h>
#include "base64.h"
#include "uart.h"

inline uint8_t Base64_Basis(uint8_t num) {
    if(num <= 25)                   return 'A' + (num - 0);
    if(num >= 26 && num <= 51)      return 'a' + (num - 26);
    if(num >= 52 && num <= 61)      return '0' + (num - 52);
    if(num == 62)                   return '+';
    if(num == 63)                   return '/';
    else                            return '?';
}

void Base64_Encode(uint8_t* data, uint16_t len) {
    uint16_t i;

    for (i = 0; i < len - 2; i += 3) {
        UART_WriteChar(Base64_Basis((data[i] >> 2) & 0x3F));
        UART_WriteChar(Base64_Basis(((data[i] & 0x3) << 4)     | ((int) (data[i + 1] & 0xF0) >> 4)));
        UART_WriteChar(Base64_Basis(((data[i + 1] & 0xF) << 2) | ((int) (data[i + 2] & 0xC0) >> 6)));
        UART_WriteChar(Base64_Basis(data[i + 2] & 0x3F));
    }

    if (i < len) {
        UART_WriteChar(Base64_Basis((data[i] >> 2) & 0x3F));
        if (i == (len - 1)) {
            UART_WriteChar(Base64_Basis(((data[i] & 0x3) << 4)));
            UART_WriteChar('=');
        } else {
            UART_WriteChar(Base64_Basis(((data[i] & 0x3) << 4) | ((int) (data[i + 1] & 0xF0) >> 4)));
            UART_WriteChar(Base64_Basis(((data[i + 1] & 0xF) << 2)));
        }
        UART_WriteChar('=');
    }
}
