#include "main.h"

uint8_t buffer[BUFFER_SIZE];
volatile uint8_t TX_Ready;

void SendPing() {
    buffer[0] = 1;
    buffer[1] = 0x10;
    buffer[2] = 8;
    buffer[3] = 1;
    buffer[4] = 'P';
    buffer[5] = 'I';
    buffer[6] = 'N';
    buffer[7] = 'G';

    UART_WriteStr("Send_started\n\r");
    SX1278_Send(buffer, 8);
}

void LORA_Init();
void OnRxError();
void SendPacket(char *string, uint8_t size);
void SendTestPackets(uint32_t number);


void main(void) {
    MCU_Init();
    UART_Init();
    UART_WriteStr("\n\n Start \n\n");
    SPI_Init();
    LORA_Init();

    UART_WriteStr("$IND \n,");
    UART_PrintHex8(SX1278_Read(REG_VERSION));
    UART_WriteChar(',');
    UART_PrintHex32(RF_FREQUENCY);
    UART_WriteChar(',');
    UART_PrintHex8(TX_OUTPUT_POWER);
    UART_WriteChar(',');
    UART_PrintHex8(LORA_BANDWIDTH);
    UART_WriteChar(',');
    UART_PrintHex8(LORA_SPREADING_FACTOR);
    UART_WriteChar(',');
    UART_PrintHex8(LORA_CODINGRATE);
    UART_WriteChar('\n');

    __delay_cycles(1000000);
    SX1278_SetRX();
    TX_Ready = 1;
    __bis_SR_register(GIE);
    SendTestPackets(100);
    while(1) {
//      SendPing();

//        __delay_cycles(16000000);
    }
}

/*
 * Interrupt
 */
#pragma vector=PORT2_VECTOR
__interrupt void Port_2_ISR(void) {
    if((P2IFG & BIT0) == BIT0) {
        UART_WriteStr("Interrupt DIO0 happens \n\r");
        SX1278_OnDIO0IRQ();
        TX_Ready = 1;
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
    UART_WriteStr("$TX Success\n");
}

void OnTxTimeout() {

}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    UART_WriteStr("Receive Finished \n");

    UART_WriteStr("RSSI: ");
    UART_PrintDec(rssi);

    UART_WriteStr(" dBm, SNR: ");
    UART_PrintHex8(snr);

    UART_WriteStr(" dB, Payload size: ");
    UART_PrintHex32(size);
    UART_WriteStr(" bytes\n");

    UART_WriteStr((char *) payload);
    UART_WriteChar('\n');
}

void OnRxTimeout() {
    UART_WriteStr("$RX Timeout\n");
}

void OnRxError() {
    UART_WriteStr("$RX Error\n");
}

void CadDone(bool cadDone) {

}

void LORA_Init() {
    SX1278_Init();
    SX1278_SetChannel(RF_FREQUENCY);

    SX1278_SetRXConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, false, 0, LORA_IQ_INVERSION_ON, true);

    SX1278_SetTXConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void SendPacket(char *string, uint8_t size)
{
    while(1)
    {
        if (TX_Ready == 1)   break; // Tam thoi su dung cho den khi co cach khac
    }
    SX1278_Send(string, size);
    TX_Ready = 0; // Tam thoi su dung cho den khi co cach khac
}
void SendTestPackets(uint32_t number)
{
    uint32_t i = 0;
    char temp[12];
    for(i = 0; i < number; i++)
    {
        sprintf(temp, "PING: %d\n\r", i);
        SendPacket(temp, 12);
//        Delay_ms(1500);
    }
}

