#ifndef MAIN_H_
#define MAIN_H_

#define Port_1  1
#define CS      BIT0
#define RESET   BIT3
#define SCLK    BIT5
#define SDI     BIT6
#define SDO     BIT7

#define Port_2  2

//#define INT1    BIT4
//#define INT2    BIT3

#define RF_FREQUENCY                        434000000 // Hz

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
#define BUFFER_SIZE                         32 // Define the payload size here

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxDone();

#endif /* MAIN_H_ */
