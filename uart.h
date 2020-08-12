#include <stdint.h>

#ifndef UART_H
#define UART_H

void UART_Init();
void UART_WriteStr(char* str);
void UART_WriteNChar(char* str, int number);
void UART_WriteChar(char character);
void UART_PrintDec(int32_t number);
void UART_PrintHex8(uint8_t number);
void UART_PrintHex32(uint32_t number);

#endif
