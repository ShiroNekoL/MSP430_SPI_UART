#include <msp430.h>
#include <stdint.h>
#include "mcu.h"
#include "uart.h"
#include "spi.h"
#include "sx1276.h"
#include "sx1276regs-fsk.h"
#include "base64.h"
#include "main.h"

uint8_t buffer[BUFFER_SIZE];
uint8_t buffer2[BUFFER_SIZE];

static RadioEvents radio_events;

int state = 0;

void SendPing() {
  buffer[0] = 1;
  buffer[1] = 0x10;
  buffer[2] = 8;
  buffer[3] = 1;
  buffer[4] = 'P';
  buffer[5] = 'I';
  buffer[6] = 'N';
  buffer[7] = 'G';

  UART_WriteStr("send_started\n\r");
//  P2IE &= ~BIT0;
  SX1278_Send(buffer, 8);
//  P2IE |= BIT0;
//  P2IFG &= ~BIT0;
//  SX1278_SetTX(10000);
}

void rf_init_lora();
void OnRxError();


void main(void) {
    MCU_Init();

    P1DIR |= BIT0;

    UART_Init();
    UART_WriteStr("\n\n Start \n\n");
    SPI_Init();
//    P2IE &= ~BIT0;
    rf_init_lora();

//    UART_WriteStr("$IND \n,");
//    UART_PrintHex8(SX1278_Read(REG_VERSION));
//    UART_WriteChar(',');
//    UART_PrintHex32(RF_FREQUENCY);
//    UART_WriteChar(',');
//    UART_PrintHex8(TX_OUTPUT_POWER);
//    UART_WriteChar(',');
//    UART_PrintHex8(LORA_BANDWIDTH);
//    UART_WriteChar(',');
//    UART_PrintHex8(LORA_SPREADING_FACTOR);
//    UART_WriteChar(',');
//    UART_PrintHex8(LORA_CODINGRATE);
//    UART_WriteChar('\n');
//    UART_WriteStr("1? \n");

//    UART_PrintHex8(SX1278_Read(0x06));
//    UART_PrintHex8(SX1278_Read(0x07));
//    UART_PrintHex8(SX1278_Read(0x08));
//    UART_WriteStr("\n2? \n");
    __delay_cycles(1000000);
//    SX1278_SetRX(100000);
//  P2IE |= BIT0;
    __bis_SR_register(GIE);
    SX1278_SetRX(10000);
//    UART_PrintHex8(SX1278_Read(REG_LR_FRFMSB));
//    UART_PrintHex8(SX1278_Read(REG_LR_FRFMID));
//    UART_PrintHex8(SX1278_Read(REG_LR_FRFLSB));
//    __delay_cycles(16000000);
//    UART_PrintHex8(SX1278_Read(REG_LR_OPMODE));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_MODEMCONFIG1));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_MODEMCONFIG2));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_MODEMCONFIG3));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_SYMBTIMEOUTLSB));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_PREAMBLEMSB));
//    UART_WriteStr("\n");
//    UART_PrintHex8(SX1278_Read(REG_LR_PREAMBLELSB));
//    UART_WriteStr("\n");
    P2IE |= BIT0;
//  while(1){ mcu_delayms(500); }
//    state = 1;
//  SendPing();
//  UART_PrintHex8(SX1278_Read(0x01));
//  UART_WriteChar('\n');
//  UART_PrintHex8(SX1278_Read(0x41));
//  __delay_cycles(1000000);
//    P2IE |= BIT0;
    while(1) {

//    if(state == 0)
//    {
////      state = 1;
//      SendPing();
//    } else if(state == 1)
//    {
//
//    }
//      SendPing();
//        SendPing();
//        __delay_cycles(16000000);
//        __delay_cycles(1000000);
//        __delay_cycles(1000000);

//        Delay_ms(1000);
//
    }
}

/*
 * INterrupt
 */
#pragma vector=PORT2_VECTOR
__interrupt void Port_2_ISR(void) {
    if((P2IFG & BIT0) == BIT0) {
        UART_WriteStr("Interrupt DIO0 happens \n\r");
        SX1278_OnDIO0IRQ();
    } else if ((P2IFG & BIT1) == BIT1) {
        UART_WriteStr("Interrupt DIO1 happens \n\r");
        SX1278_OnDIO1IRQ();
    } else if ((P2IFG & BIT2) == BIT2) {
        UART_WriteStr("Interrupt DIO2 happens \n\r");
        SX1278_OnDIO2IRQ();
    } else if ((P2IFG & BIT3) == BIT3) {
        UART_WriteStr("Interrupt DIO3 happens \n\r");
        SX1278_OnDIO3IRQ();
    } else if ((P2IFG & BIT4) == BIT4) {
        UART_WriteStr("Interrupt DIO4 happens \n\r");
        SX1278_OnDIO4IRQ();
    } else if ((P2IFG & BIT5) == BIT5) {
        UART_WriteStr("Interrupt DIO5 happens \n\r");
        SX1278_OnDIO5IRQ();
    }

    P2IFG &= 0x00;
}


void OnTxDone() {
    UART_WriteStr("$TXS\n");

//  if(state == 1) sx1276_set_rx(0);

}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {

    UART_WriteStr("txdone_start");
    P1OUT |= BIT0;

    UART_WriteStr("$RXS,");

    UART_PrintHex32(rssi);
    UART_WriteChar(',');

    UART_PrintHex8(snr);
    UART_WriteChar(',');

    UART_PrintHex32(size);
    UART_WriteChar(',');

//    Base64_Encode(payload, size);
    UART_WriteStr(payload);
    UART_WriteChar('\n');
    P1OUT &= ~BIT0;
    UART_WriteStr("????");

//  if(state == 1) SendPing();
}

void OnRxError() {
    UART_WriteStr("$RXE\n");
}

void OnRxTimeout() {
    UART_WriteStr("$RXTimeout\n");
    SX1278_SetRX(10000);
}

void rf_init_lora() {
  radio_events.TxDone = OnTxDone;
  radio_events.RxDone = OnRxDone;
  //radio_events.TxTimeout = OnTxTimeout;
  radio_events.RxTimeout = OnRxTimeout;
  radio_events.RxError = OnRxError;

  SX1278_Init(&radio_events);
  SX1278_SetChannel(RF_FREQUENCY);


  SX1278_SetTXConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  SX1278_SetRXConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                 LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                 LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 0, true, true, 50, LORA_IQ_INVERSION_ON, true);

}


