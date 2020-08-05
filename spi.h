#ifndef SPI_H
#define SPI_H

#include <msp430.h>
#include <stdint.h>
#include "main.h"

extern volatile uint8_t SPI_Buffer;

void SPI_Init();

void SPI_TXReady();
void SPI_RXReady();

void SPI_SendByte(uint8_t data);
void SPI_Receive();

void SPI_TransferByte(uint8_t data);

void SPI_ChipEnable(uint8_t ChipPort, uint8_t ChipPin);
void SPI_ChipDisable(uint8_t ChipPort, uint8_t ChipPin);

#endif
