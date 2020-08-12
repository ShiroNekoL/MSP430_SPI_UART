#ifndef SX1276_H
#define SX1276_H

#include <stdint.h>
#include <stdbool.h>
#include <msp430.h>
#include <string.h>
#include <math.h>
#include "mcu.h"
#include "sx1276.h"
#include "sx1276regs-fsk.h"
#include "sx1276regs-lora.h"
#include "spi.h"
#include "uart.h"
#include "main.h"

//typedef struct {
//    void (*TxDone) ();
//    void (*TxTimeout) ();
//    void (*RxDone) (uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
//    void (*RxTimeout) ();
//    void (*RxError) ();
//    void (*FhssChangeChannel) (uint8_t currentChannel);
//    void (*CadDone) (int8_t channelActivityDetected);
//} RadioEvents;

typedef enum {
    RF_IDLE = 0,
    RF_RX_RUNNING,
    RF_TX_RUNNING,
    RF_CAD,
} RadioStates;

typedef enum {
    MODEM_FSK = 0,
    MODEM_LORA,
} RadioModem;

typedef struct {
    int8_t                          Power;
    uint32_t                        Fdev;
    uint32_t                        Bandwidth;
    uint32_t                        BandwidthAfc;
    uint32_t                        Datarate;
    uint16_t                        PreambleLen;
    bool                            FixLen;
    uint8_t                         PayloadLen;
    bool                            CrcOn;
    bool                            IqInverted;
    bool                            RxContinuous;
    uint32_t                        TxTimeout;
} RadioFSKSettings;

typedef struct {
    uint8_t                         PreambleDetected;
    uint8_t                         SyncWordDetected;
    int8_t                          RssiValue;
    int32_t                         AfcValue;
    uint8_t                         RxGain;
    uint16_t                        Size;
    uint16_t                        NbBytes;
    uint8_t                         FifoThresh;
    uint8_t                         ChunkSize;
} RadioFSKPacketHandler;

typedef struct {
    int8_t                          Power;
    uint32_t                        Bandwidth;
    uint32_t                        Datarate;               // Spreading Factor
    bool                            LowDatarateOptimize;
    uint8_t                         Coderate;
    uint16_t                        PreambleLen;
    bool                            FixLen;
    uint8_t                         PayloadLen;
    bool                            CrcOn;
    bool                            FreqHopOn;
    uint8_t                         HopPeriod;
    bool                            IqInverted;
    bool                            RxContinuous;
    uint32_t                        TxTimeout;
} RadioLORASettings;

typedef struct {
    int8_t                          SnrValue;
    int16_t                         RssiValue;
    uint8_t                         Size;
} RadioLORAPacketHandler;

typedef struct {
    RadioStates                     State;
    RadioModem                      Modem;
    uint32_t                        Channel;
    RadioFSKSettings                Fsk;
    RadioFSKPacketHandler           FskPacketHandler;
    RadioLORASettings               LoRa;
    RadioLORAPacketHandler          LoRaPacketHandler;
} RadioSettings;

typedef struct sx1278_struct {
    RadioSettings Settings;
} SX1278;

#define XTAL_FREQ                   32000000
#define FREQ_STEP                   61.03515625             // 32x10^6 / 2^19, reference at page 109

#define RX_BUFFER_SIZE              256

#define RSSI_OFFSET_LF              -164
#define RSSI_OFFSET_HF              -157

extern SX1278 sx1278;

void SX1278_Init();                                                                 // Done
void SX1278_Reset(uint8_t ResetPort, uint8_t ResetPin);                             // Done
void SX1278_RXChainCalib();                                                         // Done

void SX1278_SetRXConfig(RadioModem modem, uint32_t bandwidth,                       // Done
                        uint32_t datarate, uint8_t coderate,
                        uint32_t bandwidthAfc, uint16_t preambleLen,
                        uint16_t symbTimeout, bool fixLen,
                        uint8_t payloadLen,
                        bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                        bool iqInverted, bool rxContinuous);

void SX1278_SetTXConfig(RadioModem modem, int8_t power, uint32_t fdev,              // Semi-done
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout);

uint32_t SX1278_GetTimeOnAir(RadioModem modem, uint8_t pktLen);                     // Done

void SX1278_SetChannel(uint32_t freq);                                              // Done
void SX1278_SetModem(RadioModem modem);                                             // Done
void SX1278_SetOpmode(uint8_t opmode);                                              // Done
void SX1278_SetRX();                                                                // Done without Timeout
void SX1278_SetTX();                                                                // Done without Timeout

void SX1278_Send(uint8_t *buffer, uint8_t size);

void SX1278_Write(uint8_t addr, uint8_t data);                                      // Done
void SX1278_WriteBuffer(uint8_t addr, uint8_t* data, uint8_t len);                  // Done
void SX1278_WriteFIFO(uint8_t *data, uint8_t len);                                  // Done

uint8_t SX1278_Read(uint8_t addr);                                                  // Done
void SX1278_ReadBuffer(uint8_t addr, uint8_t* buffer, uint8_t len);                 // Done
void SX1278_ReadFIFO(uint8_t *data, uint8_t len);                                   // Done

// ISR from DIO
void SX1278_OnDIO0IRQ(void);                                                        // Done
void SX1278_OnDIO1IRQ(void);                                                        // Done
void SX1278_OnDIO2IRQ(void);                                                        // Done
void SX1278_OnDIO3IRQ(void);                                                        // Done
void SX1278_OnDIO4IRQ(void);                                                        // Done
void SX1278_OnDIO5IRQ(void);                                                        // Done


#endif
