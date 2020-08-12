#ifndef MAIN_H_
#define MAIN_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcu.h"
#include "uart.h"
#include "spi.h"
#include "sx1276.h"
#include "sx1276regs-fsk.h"
#include "base64.h"
#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#define Port_1  1
#define CS      BIT0
#define RESET   BIT3
#define SCLK    BIT5
#define SDI     BIT6
#define SDO     BIT7

#define Port_2  2
#define DIO0    BIT0
#define DIO1    BIT1
#define DIO2    BIT2
#define DIO3    BIT3
#define DIO4    BIT4
#define DIO5    BIT5

#define RF_FREQUENCY                        434000000 // Hz
#define RF_MID_BAND_THRESH                  525000000 // Hz

#define FSK_FDEV                            25e3      // Hz
#define FSK_DATARATE                        50e3      // bps
#define FSK_BANDWIDTH                       50e3      // Hz
#define FSK_AFC_BANDWIDTH                   83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH                 5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON           false

#define LORA_BANDWIDTH                      0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
// Currently having trouble with SF6
#define LORA_SPREADING_FACTOR               12        // [SF7..SF12]
#define LORA_CODINGRATE                     1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                8         // Same for Tx and Rx
// Ts = symbol_duration = 2^SF / bandwidth / Timeout = SymbolTimeout * Ts
// Currently Timeout ~ 2^12 / 125k * 30 ~ 1s
#define LORA_SYMBOL_TIMEOUT                 30         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON          false
#define LORA_IQ_INVERSION_ON                false

#define RX_TIMEOUT_VALUE                    1000
#define TX_OUTPUT_POWER                     14        // dBm
#define BUFFER_SIZE                         32        // Define the payload size here

void OnTxDone();
void OnTxTimeout();
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout();
void OnRxError();
void CadDone(bool cadDone);
void LORA_Init();

#endif /* MAIN_H_ */
