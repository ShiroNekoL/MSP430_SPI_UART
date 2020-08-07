#include "sx1276.h"

static uint8_t RxTxBuffer[RX_BUFFER_SIZE];
SX1278 sx1278;

static RadioEvents* radio_events;

typedef struct {
    RadioModem    Modem;
    uint8_t       Addr;
    uint8_t       Value;
} RadioRegisters;

const RadioRegisters radio_registers[] = {
    { MODEM_FSK , REG_LNA                 , 0x23 },
    { MODEM_FSK , REG_RXCONFIG            , 0x1E },
    { MODEM_FSK , REG_RSSICONFIG          , 0xD2 },
    { MODEM_FSK , REG_AFCFEI              , 0x01 },
    { MODEM_FSK , REG_PREAMBLEDETECT      , 0xAA },
    { MODEM_FSK , REG_OSC                 , 0x07 },
    { MODEM_FSK , REG_SYNCCONFIG          , 0x12 },
    { MODEM_FSK , REG_SYNCVALUE1          , 0xC1 },
    { MODEM_FSK , REG_SYNCVALUE2          , 0x94 },
    { MODEM_FSK , REG_SYNCVALUE3          , 0xC1 },
    { MODEM_FSK , REG_PACKETCONFIG1       , 0xD8 },
    { MODEM_FSK , REG_FIFOTHRESH          , 0x8F },
    { MODEM_FSK , REG_IMAGECAL            , 0x02 },
    { MODEM_FSK , REG_DIOMAPPING1         , 0x00 },
    { MODEM_FSK , REG_DIOMAPPING2         , 0x30 },
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH , 0x40 },
};

typedef struct {
    uint32_t bandwidth;
    uint8_t  RegValue;
} FSKBandwith;

const FSKBandwith fsk_bandwidths[] = {
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Badwidth
};

#define RF_MID_BAND_THRESH                          525000000

uint32_t Lora_channel[10] = {
    432000000,
    432500000,
    433000000,
    433500000,
    434000000,
    434500000,
    435000000,
    435500000,
    436000000,
    436500000
};

void SX1278_ChangeFHSS(uint8_t index) {
    SX1278_SetChannel(Lora_channel[index %= 10]);
}

static uint8_t SX1278_GetPASelect(uint32_t channel) {
    if(channel < RF_MID_BAND_THRESH )
        return RF_PACONFIG_PASELECT_PABOOST;
    else
        return RF_PACONFIG_PASELECT_PABOOST;
}

static uint8_t SX1278_GetFSKBandwithVal(uint32_t bandwidth) {
    uint8_t i;
    for(i = 0; i < (sizeof(fsk_bandwidths) / sizeof(FSKBandwith)) - 1; i++) {
        if((bandwidth >= fsk_bandwidths[i].bandwidth) && (bandwidth < fsk_bandwidths[i + 1].bandwidth)) {
            return fsk_bandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while(1);
}

void SX1278_Init(RadioEvents* events) {

    radio_events = events;

    // Initialize driver timeout timers
    //TimerInit( &TxTimeoutTimer, sx1278OnTimeoutIrq );
    //TimerInit( &RxTimeoutTimer, sx1278OnTimeoutIrq );
    //TimerInit( &RxTimeoutSyncWord, sx1278OnTimeoutIrq );

    SX1278_Reset(Port_1, RESET);

    SX1278_RXChainCalib();

    SX1278_SetOpmode(RF_OPMODE_SLEEP);

    // Initialize IRQ interrupts
//    P2DIR &= (~BIT2); // Set P2.1 SEL as Input
//    P2IES &= (~BIT2); // Rising edge 0 -> 1
//    P2IE  |=  (BIT2); // Enable interrupt for P2.1
//    P2IFG &= (~BIT2); // Clear interrupt flag for P2.1

    __enable_interrupt();
    uint8_t i;
    for(i = 0; i < sizeof(radio_registers) / sizeof(RadioRegisters); i++) {
        SX1278_SetModem(radio_registers[i].Modem);
        SX1278_Write(radio_registers[i].Addr, radio_registers[i].Value);
    }

    // Default initialization, need to change the modem mode later
    SX1278_SetModem(MODEM_FSK);

    sx1278.Settings.State = RF_IDLE;
}

void SX1278_Reset(uint8_t ResetPort, uint8_t ResetPin) {
    Delay_ms(1);

    if (ResetPort == Port_1) {
        P1DIR |= ResetPin;
        P1OUT |= ResetPin;
    } else if (ResetPort == Port_2) {
        P2DIR |= ResetPin;
        P2OUT |= ResetPin;
    }

    Delay_ms(1);

    if (ResetPort == Port_1) {
        P1DIR &= ~ResetPin;
        P1OUT &= ~ResetPin;
    } else if (ResetPort == Port_2) {
        P2DIR &= ~ResetPin;
        P2OUT &= ~ResetPin;
    }

    Delay_ms(6);
}

void SX1278_RXChainCalib() {
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = SX1278_Read(REG_PACONFIG);
    initialFreq = (double)(((uint32_t) SX1278_Read(REG_FRFMSB) << 16) |
                           ((uint32_t) SX1278_Read(REG_FRFMID) << 8 ) |
                           ((uint32_t) SX1278_Read(REG_FRFLSB)      )) * (double) FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    SX1278_Write(REG_PACONFIG, 0x00);

    // Launch Rx chain calibration for LF band
    // Mask ImageCalStart bit in RegImageCal register and set the ImageCalStart bit
    SX1278_Write(REG_IMAGECAL, (SX1278_Read(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
    // Check if ImageCalRunning is equal to 1, if it's not, the calibration is finished
    while((SX1278_Read(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) {
    }

    // Sets a Frequency in HF band
    SX1278_SetChannel(868000000);

    // Launch Rx chain calibration for HF band
    // Mask ImageCalStart bit in RegImageCal register and set the ImageCalStart bit
    SX1278_Write(REG_IMAGECAL, (SX1278_Read(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
    // Check if ImageCalRunning is equal to 1, if it's not, the calibration is finished
    while((SX1278_Read(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) {
    }

    // Restore context
    SX1278_Write(REG_PACONFIG, regPaConfigInitVal);
    SX1278_SetChannel(initialFreq);
}

void SX1278_SetRXConfig(RadioModem modem, uint32_t bandwidth,
                        uint32_t datarate, uint8_t coderate,
                        uint32_t bandwidthAfc, uint16_t preambleLen,
                        uint16_t symbTimeout, bool fixLen,
                        uint8_t payloadLen,
                        bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                        bool iqInverted, bool rxContinuous) {
    SX1278_SetModem(modem);

    switch(modem) {
        // Unchecked
        case MODEM_FSK: {
            sx1278.Settings.Fsk.Bandwidth =     bandwidth;
            sx1278.Settings.Fsk.Datarate =      datarate;
            sx1278.Settings.Fsk.BandwidthAfc =  bandwidthAfc;
            sx1278.Settings.Fsk.FixLen =        fixLen;
            sx1278.Settings.Fsk.PayloadLen =    payloadLen;
            sx1278.Settings.Fsk.CrcOn =         crcOn;
            sx1278.Settings.Fsk.IqInverted =    iqInverted;
            sx1278.Settings.Fsk.RxContinuous =  rxContinuous;
            sx1278.Settings.Fsk.PreambleLen =   preambleLen;

            datarate = (uint16_t) ((double) XTAL_FREQ / (double) datarate );
            SX1278_Write(REG_BITRATEMSB, (uint8_t) (datarate >> 8));
            SX1278_Write(REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

            SX1278_Write(REG_RXBW, SX1278_GetFSKBandwithVal(bandwidth));
            SX1278_Write(REG_AFCBW, SX1278_GetFSKBandwithVal(bandwidthAfc));

            SX1278_Write(REG_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8) & 0xFF));
            SX1278_Write(REG_PREAMBLELSB, (uint8_t) (preambleLen & 0xFF));

            if(fixLen == 1) {
                SX1278_Write(REG_PAYLOADLENGTH, payloadLen);
            } else {
                SX1278_Write(REG_PAYLOADLENGTH, 0xFF); // Set payload length to the maximum
            }

            SX1278_Write(REG_PACKETCONFIG1,
                         (SX1278_Read(REG_PACKETCONFIG1) & RF_PACKETCONFIG1_CRC_MASK & RF_PACKETCONFIG1_PACKETFORMAT_MASK) |
                         ((fixLen == 1) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE) |
                         (crcOn << 4));
            }
            break;
        case MODEM_LORA: {
            if(bandwidth > 2) {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while(1);
            }
            bandwidth += 7;

            sx1278.Settings.LoRa.Bandwidth =     bandwidth;
            sx1278.Settings.LoRa.Datarate =      datarate;
            sx1278.Settings.LoRa.Coderate =      coderate;
            sx1278.Settings.LoRa.PreambleLen =   preambleLen;
            sx1278.Settings.LoRa.FixLen =        fixLen;
            sx1278.Settings.LoRa.PayloadLen =    payloadLen;
            sx1278.Settings.LoRa.CrcOn =         crcOn;
            sx1278.Settings.LoRa.FreqHopOn =     freqHopOn;
            sx1278.Settings.LoRa.HopPeriod =     hopPeriod;
            sx1278.Settings.LoRa.IqInverted =    iqInverted;
            sx1278.Settings.LoRa.RxContinuous =  rxContinuous;

            if(datarate > 12)     datarate = 12;
            else if(datarate < 6) datarate = 6;

            if(((bandwidth == 7) && ((datarate == 11) || (datarate == 12))) || ((bandwidth == 8) && (datarate == 12))) {
                sx1278.Settings.LoRa.LowDatarateOptimize = 0x01;
            } else {
                sx1278.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            // Mask RegModemConfig1 bits and set the parameter accordingly / Reference at page 113
            SX1278_Write(REG_LR_MODEMCONFIG1,
                         (SX1278_Read(REG_LR_MODEMCONFIG1) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) |
                         (bandwidth << 4) | (coderate << 1) |
                         fixLen);

            // Mask RegModemConfig2's SpreadingFactor, RxPayloadCrcOn and SymbTimeout bits
            // Set those parameter accordingly / Reference at page 113
            SX1278_Write(REG_LR_MODEMCONFIG2,
                         (SX1278_Read(REG_LR_MODEMCONFIG2) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK) |
                         (datarate << 4) | (crcOn << 2) |
                         ((symbTimeout >> 8) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK));

            // Mask RegModemConfig3's LowDataRateOptimize bits
            // Set the parameter accordingly / Reference at page 114
            SX1278_Write(REG_LR_MODEMCONFIG3,
                         (SX1278_Read(REG_LR_MODEMCONFIG3) & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK) |
                         (sx1278.Settings.LoRa.LowDatarateOptimize << 3));

            // Set the lowest 8 bits to the register / Reference at page 114
            SX1278_Write(REG_LR_SYMBTIMEOUTLSB, (uint8_t) (symbTimeout & 0xFF));

            // Set the preamble length / Reference at page 114
            SX1278_Write(REG_LR_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8 ) & 0xFF));
            SX1278_Write(REG_LR_PREAMBLELSB, (uint8_t) ((preambleLen      ) & 0xFF));

            // Set the PayloadLength if it's in implicit header mode / Reference at page 114
            if(fixLen == 1) SX1278_Write(REG_LR_PAYLOADLENGTH, payloadLen);

            // Check Frequency Hopping for fast frequency hop
            if(sx1278.Settings.LoRa.FreqHopOn == true) {
                SX1278_Write(REG_LR_PLLHOP, (SX1278_Read(REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK) | RFLR_PLLHOP_FASTHOP_ON);
                SX1278_Write(REG_LR_HOPPERIOD, sx1278.Settings.LoRa.HopPeriod);
            }

            // Check Errata note for more information/ Reference at page 4 of the Errata Note
            if((bandwidth == 9) && (sx1278.Settings.Channel > RF_MID_BAND_THRESH)) {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278_Write(REG_LR_TEST36, 0x02);
                SX1278_Write(REG_LR_TEST3A, 0x64);
            } else if(bandwidth == 9) {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278_Write(REG_LR_TEST36, 0x02);
                SX1278_Write(REG_LR_TEST3A, 0x7F);
            } else {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278_Write(REG_LR_TEST36, 0x03);
            }

            // Config the RX in the special case of SF = 6
            if(datarate == 6) {
                SX1278_Write(REG_LR_DETECTOPTIMIZE,
                             (SX1278_Read(REG_LR_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) |
                             RFLR_DETECTIONOPTIMIZE_SF6);
                SX1278_Write(REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6);
            } else {
                SX1278_Write(REG_LR_DETECTOPTIMIZE,
                             (SX1278_Read(REG_LR_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
                SX1278_Write(REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12);
            }
            }
            break;
    }
}

void SX1278_SetTXConfig(RadioModem modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout) {
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    SX1278_SetModem(modem);

    paConfig = SX1278_Read(REG_PACONFIG);
    paDac = SX1278_Read(REG_PADAC);

    paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK)  | SX1278_GetPASelect(sx1278.Settings.Channel);
    paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK) | 0x70;

    // Currently unchecked / Can't find in the Datasheet
    if((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST) {
        if(power > 17) paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
        else           paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;

        if((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
            if (power < 5)  power = 5;
            if (power > 20) power = 20;
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 5) & 0x0F);
        } else {
            if (power < 2) power = 2;
            if (power > 17) power = 17;
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 2) & 0x0F);
        }
    } else {
        if(power < -1) power = -1;
        if(power > 14) power = 14;
        paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power + 1) & 0x0F);
    }

    SX1278_Write(REG_PACONFIG, paConfig);
    SX1278_Write(REG_PADAC, paDac);

    switch(modem) {
        // Unchecked for FSK
        case MODEM_FSK: {
            sx1278.Settings.Fsk.Power =        power;
            sx1278.Settings.Fsk.Fdev =         fdev;
            sx1278.Settings.Fsk.Bandwidth =    bandwidth;
            sx1278.Settings.Fsk.Datarate =     datarate;
            sx1278.Settings.Fsk.PreambleLen =  preambleLen;
            sx1278.Settings.Fsk.FixLen =       fixLen;
            sx1278.Settings.Fsk.CrcOn =        crcOn;
            sx1278.Settings.Fsk.IqInverted =   iqInverted;
            sx1278.Settings.Fsk.TxTimeout =    timeout;

            fdev = (uint16_t) ((double) fdev / (double) FREQ_STEP);
            SX1278_Write(REG_FDEVMSB, (uint8_t) (fdev >> 8));
            SX1278_Write(REG_FDEVLSB, (uint8_t) (fdev & 0xFF));

            datarate = (uint16_t) ((double) XTAL_FREQ / (double) datarate);
            SX1278_Write(REG_BITRATEMSB, (uint8_t) (datarate >> 8));
            SX1278_Write(REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

            SX1278_Write(REG_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
            SX1278_Write(REG_PREAMBLELSB, (preambleLen     ) & 0xFF);

            SX1278_Write(REG_PACKETCONFIG1,
                         (SX1278_Read(REG_PACKETCONFIG1) & RF_PACKETCONFIG1_CRC_MASK & RF_PACKETCONFIG1_PACKETFORMAT_MASK) |
                         ((fixLen == 1) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE) |
                         (crcOn << 4));

            }
            break;
        case MODEM_LORA: {
            sx1278.Settings.LoRa.Power = power;

            if( bandwidth > 2 ) {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;

            sx1278.Settings.LoRa.Bandwidth =    bandwidth;
            sx1278.Settings.LoRa.Datarate =     datarate;
            sx1278.Settings.LoRa.Coderate =     coderate;
            sx1278.Settings.LoRa.PreambleLen =  preambleLen;
            sx1278.Settings.LoRa.FixLen =       fixLen;
            sx1278.Settings.LoRa.FreqHopOn =    freqHopOn;
            sx1278.Settings.LoRa.HopPeriod =    hopPeriod;
            sx1278.Settings.LoRa.CrcOn =        crcOn;
            sx1278.Settings.LoRa.IqInverted =   iqInverted;
            sx1278.Settings.LoRa.TxTimeout =    timeout;

            if(datarate > 12) datarate = 12;
            else if(datarate < 6) datarate = 6;

            if(((bandwidth == 7) && ((datarate == 11) || (datarate == 12))) || ((bandwidth == 8) && (datarate == 12))) {
                sx1278.Settings.LoRa.LowDatarateOptimize = 0x01;
            } else {
                sx1278.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            if(sx1278.Settings.LoRa.FreqHopOn == true) {
                SX1278_Write(REG_LR_PLLHOP, (SX1278_Read(REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK) | RFLR_PLLHOP_FASTHOP_ON);
                SX1278_Write(REG_LR_HOPPERIOD, sx1278.Settings.LoRa.HopPeriod);
            }

            SX1278_Write(REG_LR_MODEMCONFIG1,
                         (SX1278_Read(REG_LR_MODEMCONFIG1) & RFLR_MODEMCONFIG1_BW_MASK &  RFLR_MODEMCONFIG1_CODINGRATE_MASK & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) |
                         (bandwidth << 4) | (coderate << 1) |
                         fixLen);

            SX1278_Write(REG_LR_MODEMCONFIG2,
                         (SX1278_Read(REG_LR_MODEMCONFIG2) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) |
                         (datarate << 4) | (crcOn << 2));

            SX1278_Write(REG_LR_MODEMCONFIG3,
                         (SX1278_Read( REG_LR_MODEMCONFIG3 ) & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                         (sx1278.Settings.LoRa.LowDatarateOptimize << 3));

            SX1278_Write(REG_LR_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
            SX1278_Write(REG_LR_PREAMBLELSB, (preambleLen     ) & 0xFF);

            if(datarate == 6) {
                SX1278_Write(REG_LR_DETECTOPTIMIZE,
                             (SX1278_Read(REG_LR_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) |
                             RFLR_DETECTIONOPTIMIZE_SF6);
                SX1278_Write(REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6);
            } else {
                SX1278_Write(REG_LR_DETECTOPTIMIZE,
                             (SX1278_Read(REG_LR_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
                SX1278_Write(REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12);
            }
            }
            break;
    }
}

uint32_t SX1278_GetTimeOnAir(RadioModem modem, uint8_t pktLen) {
    uint32_t airTime = 0;

    switch (modem) {
        case MODEM_FSK: {
            // Currently unchecked / Can't be found in the Semtech datasheet
            airTime = round((8 * (sx1278.Settings.Fsk.PreambleLen +
                            ((SX1278_Read(REG_SYNCCONFIG) & ~RF_SYNCCONFIG_SYNCSIZE_MASK) + 1) +
                            ((sx1278.Settings.Fsk.FixLen == 0x01) ? 0.0 : 1.0) +
                            (((SX1278_Read(REG_PACKETCONFIG1) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK) != 0x00) ? 1.0 : 0) +
                            pktLen +
                            ((sx1278.Settings.Fsk.CrcOn == 0x01) ? 2.0 : 0)) / sx1278.Settings.Fsk.Datarate) * 1e3);
            }
            break;
        case MODEM_LORA: {
            double bandwidth = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            // Bandwidth reference at RegModemConfig1, page 112
            // Bandwidth = Chip rate
            switch(sx1278.Settings.LoRa.Bandwidth) {
                case 7: // 125 kHz
                    bandwidth = 125e3;
                    break;
                case 8: // 250 kHz
                    bandwidth = 250e3;
                    break;
                case 9: // 500 kHz
                    bandwidth = 500e3;
                    break;
            }

            // Symbol duration = 2^SF / bandwidth : time for one symbol (secs)
            double symbol_rate = bandwidth / (1 << sx1278.Settings.LoRa.Datarate);
            double symbol_duration = 1 / symbol_rate;
            // Time of preamble / Reference at page 31
            double tPreamble = (sx1278.Settings.LoRa.PreambleLen + 4.25) * symbol_duration;
            // Symbol length of payload and time of symbol
            double tmp = ceil((8 * pktLen - 4 * sx1278.Settings.LoRa.Datarate +
                              28 + 16 * sx1278.Settings.LoRa.CrcOn -
                              (sx1278.Settings.LoRa.FixLen ? 20 : 0)) /
                              (double) (4 * (sx1278.Settings.LoRa.Datarate -
                              ((sx1278.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0)))) *
                              (sx1278.Settings.LoRa.Coderate + 4);
            double nPayload = 8 + (( tmp > 0 ) ? tmp : 0);
            double tPayload = nPayload * symbol_duration;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return us seconds
            airTime = floor(tOnAir * 1e3 + 0.999);
            }
            break;
    }
    return airTime;
}

void SX1278_Send(uint8_t *buffer, uint8_t size) {
    uint32_t txTimeout = 0;

    switch( sx1278.Settings.Modem ) {
        case MODEM_FSK: {
            sx1278.Settings.FskPacketHandler.NbBytes = 0;
            sx1278.Settings.FskPacketHandler.Size = size;

            if(sx1278.Settings.Fsk.FixLen == false) {
                SX1278_WriteFIFO((uint8_t*) &size, 1);
            } else {
                SX1278_Write(REG_PAYLOADLENGTH, size);
            }

            if((size > 0) && (size <= 64)) {
                sx1278.Settings.FskPacketHandler.ChunkSize = size;
            } else {
                MCU_MemCpy(RxTxBuffer, buffer, size);
                sx1278.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            SX1278_WriteFIFO(buffer, sx1278.Settings.FskPacketHandler.ChunkSize);
            sx1278.Settings.FskPacketHandler.NbBytes += sx1278.Settings.FskPacketHandler.ChunkSize;
            txTimeout = sx1278.Settings.Fsk.TxTimeout;
            }
            break;
        case MODEM_LORA: {
            if( sx1278.Settings.LoRa.IqInverted == true ) {
                SX1278_Write(REG_LR_INVERTIQ, ((SX1278_Read(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON));
                SX1278_Write(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            } else {
                SX1278_Write(REG_LR_INVERTIQ, ((SX1278_Read(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
                SX1278_Write(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            sx1278.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            SX1278_Write(REG_LR_PAYLOADLENGTH, size);

            // Full buffer used for Tx
            SX1278_Write(REG_LR_FIFOTXBASEADDR, 0);
            SX1278_Write(REG_LR_FIFOADDRPTR, 0);

            // FIFO operations can not take place in Sleep mode
            if((SX1278_Read(REG_OPMODE) & ~RF_OPMODE_MASK) == RF_OPMODE_SLEEP) {
                SX1278_SetOpmode(RF_OPMODE_STANDBY);
                sx1278.Settings.State = RF_IDLE;

                Delay_ms(1);
            }
            // Write payload buffer
            SX1278_WriteFIFO( buffer, size );
            txTimeout = sx1278.Settings.LoRa.TxTimeout;
            }
            break;
    }

    SX1278_SetTX(txTimeout);
}

void SX1278_SetRX(uint32_t timeout) {
    int8_t rxContinuous = false;

    switch(sx1278.Settings.Modem) {
        // Unchecked for FSK
        case MODEM_FSK: {
            rxContinuous = sx1278.Settings.Fsk.RxContinuous;

            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
                                                                          RF_DIOMAPPING1_DIO1_MASK &
                                                                          RF_DIOMAPPING1_DIO2_MASK) |
                                                                          RF_DIOMAPPING1_DIO0_00 |
                                                                          RF_DIOMAPPING1_DIO1_00 |
                                                                          RF_DIOMAPPING1_DIO2_11);

            SX1278_Write(REG_DIOMAPPING2, (SX1278_Read(REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
                                                                          RF_DIOMAPPING2_MAP_MASK) |
                                                                          RF_DIOMAPPING2_DIO4_11 |
                                                                          RF_DIOMAPPING2_MAP_PREAMBLEDETECT);

            sx1278.Settings.FskPacketHandler.FifoThresh = SX1278_Read(REG_FIFOTHRESH) & 0x3F;

            SX1278_Write(REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT);

            sx1278.Settings.FskPacketHandler.PreambleDetected = false;
            sx1278.Settings.FskPacketHandler.SyncWordDetected = false;
            sx1278.Settings.FskPacketHandler.NbBytes = 0;
            sx1278.Settings.FskPacketHandler.Size = 0;
            }
            break;
        case MODEM_LORA: {
            // Check IQ invert, set IQ invert for RX only
            if(sx1278.Settings.LoRa.IqInverted == true) {
                SX1278_Write(REG_LR_INVERTIQ, ((SX1278_Read(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF));
                SX1278_Write(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
            } else {
                SX1278_Write(REG_LR_INVERTIQ, ((SX1278_Read(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
                SX1278_Write(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
            }

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if(sx1278.Settings.LoRa.Bandwidth < 9) {
                SX1278_Write(REG_LR_DETECTOPTIMIZE, SX1278_Read(REG_LR_DETECTOPTIMIZE) & 0x7F);
                SX1278_Write(REG_LR_TEST30, 0x00);
                switch(sx1278.Settings.LoRa.Bandwidth) {
                    case 0: // 7.8 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x48);
                        SX1278_SetChannel(sx1278.Settings.Channel + 7.81e3);
                        break;
                    case 1: // 10.4 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x44);
                        SX1278_SetChannel(sx1278.Settings.Channel + 10.42e3);
                        break;
                    case 2: // 15.6 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x44);
                        SX1278_SetChannel(sx1278.Settings.Channel + 15.62e3);
                        break;
                    case 3: // 20.8 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x44);
                        SX1278_SetChannel(sx1278.Settings.Channel + 20.83e3);
                        break;
                    case 4: // 31.2 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x44);
                        SX1278_SetChannel(sx1278.Settings.Channel + 31.25e3);
                        break;
                    case 5: // 41.4 kHz
                        SX1278_Write(REG_LR_TEST2F, 0x44);
                        SX1278_SetChannel(sx1278.Settings.Channel + 41.67e3);
                        break;
                    case 6: // 62.5 kHz
                        //SX1278_Write(REG_LR_TEST2F, 0x40);
                        break;
                    case 7: // 125 kHz
                        //SX1278_Write(REG_LR_TEST2F, 0x40);
                        break;
                    case 8: // 250 kHz
                        //SX1278_Write(REG_LR_TEST2F, 0x40);
                        break;
                }
            } else {
                SX1278_Write(REG_LR_DETECTOPTIMIZE, SX1278_Read(REG_LR_DETECTOPTIMIZE) | 0x80);
            }

            rxContinuous = sx1278.Settings.LoRa.RxContinuous;

            // Check if Frequency Hopping is enabled / if it's not, set the FHSS bit to clear it
            if(sx1278.Settings.LoRa.FreqHopOn == true) {
                SX1278_Write(REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                             //RFLR_IRQFLAGS_RXDONE |
                             //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                             RFLR_IRQFLAGS_VALIDHEADER |
                             RFLR_IRQFLAGS_TXDONE |
                             RFLR_IRQFLAGS_CADDONE |
                             //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                             RFLR_IRQFLAGS_CADDETECTED);

                // DIO0=RxDone, DIO2=FhssChangeChannel
                SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00);
            } else {
                SX1278_Write(REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                             //RFLR_IRQFLAGS_RXDONE |
                             //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                             RFLR_IRQFLAGS_VALIDHEADER |
                             RFLR_IRQFLAGS_TXDONE |
                             RFLR_IRQFLAGS_CADDONE |
                             RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                             RFLR_IRQFLAGS_CADDETECTED);

                // DIO0=RxDone
                SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_00);
            }
            SX1278_Write(REG_LR_FIFORXBASEADDR, 0);
            SX1278_Write(REG_LR_FIFOADDRPTR, 0);
        }
        break;
    }

    // Initialize the buffer array to 0
    memset(RxTxBuffer, 0, (size_t) RX_BUFFER_SIZE);

    sx1278.Settings.State = RF_RX_RUNNING;
    if(timeout != 0) {
        //TimerSetValue(&RxTimeoutTimer, timeout);
        //TimerStart(&RxTimeoutTimer);
    }

    if(sx1278.Settings.Modem == MODEM_FSK) {
        SX1278_SetOpmode(RF_OPMODE_RECEIVER);

        if(rxContinuous == false) {
            //TimerSetValue( &RxTimeoutSyncWord, ceil( ( 8.0 * ( sx1278.Settings.Fsk.PreambleLen +
            //                                                 ( ( sx1278Read( REG_SYNCCONFIG ) &
            //                                                    ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
            //                                                    1.0 ) + 10.0 ) /
            //                                                 ( double )sx1278.Settings.Fsk.Datarate ) * 1e3 ) + 4 );
            //TimerStart( &RxTimeoutSyncWord );
        }
    } else {
        if(rxContinuous == true) {
            SX1278_SetOpmode(RFLR_OPMODE_RECEIVER);
        } else {
            SX1278_SetOpmode(RFLR_OPMODE_RECEIVER_SINGLE);
        }
    }
}

void SX1278_SetTX(uint32_t timeout) {
    //TimerSetValue(&TxTimeoutTimer, timeout);

    switch(sx1278.Settings.Modem) {
        // Unchecked for FSK
        case MODEM_FSK: {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK & RF_DIOMAPPING1_DIO1_MASK & RF_DIOMAPPING1_DIO2_MASK) | RF_DIOMAPPING1_DIO1_01);
            SX1278_Write(REG_DIOMAPPING2, (SX1278_Read(REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK & RF_DIOMAPPING2_MAP_MASK));
            sx1278.Settings.FskPacketHandler.FifoThresh = SX1278_Read(REG_FIFOTHRESH) & 0x3F;
            }
            break;
        case MODEM_LORA: {
            // Check if Frequency Hopping is enabled / if it's not, set the FHSS bit to clear it
            if(sx1278.Settings.LoRa.FreqHopOn == true) {
                SX1278_Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED);

                // DIO0=TxDone, DIO2=FhssChangeChannel
                SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00);
            } else {
                SX1278_Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED);

                // DIO0=TxDone
                SX1278_Write(REG_DIOMAPPING1, (SX1278_Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_01);
            }
            }
            break;
    }

    sx1278.Settings.State = RF_TX_RUNNING;
    //TimerStart( &TxTimeoutTimer );
    SX1278_SetOpmode(RF_OPMODE_TRANSMITTER);
}


void SX1278_SetChannel(uint32_t freq) {
    sx1278.Settings.Channel = freq;
    freq = (uint32_t) ((double) freq / (double) FREQ_STEP);             // Calculate frequency in 32 bit
    SX1278_Write(REG_FRFMSB, (uint8_t) ((freq >> 16) & 0xFF));
    SX1278_Write(REG_FRFMID, (uint8_t) ((freq >> 8 ) & 0xFF));
    SX1278_Write(REG_FRFLSB, (uint8_t) ((freq      ) & 0xFF));
}

void SX1278_SetModem(RadioModem modem) {
    if(sx1278.Settings.Modem == modem) return;

    sx1278.Settings.Modem = modem;

    switch(sx1278.Settings.Modem) {
        default:
        case MODEM_FSK:
            SX1278_SetOpmode(RF_OPMODE_SLEEP);
            // Mask LongRangeMode bit and set it to FSK mode / Reference at page 108
            SX1278_Write(REG_OPMODE, (SX1278_Read(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK) | RFLR_OPMODE_LONGRANGEMODE_OFF);

            // Default DIO mapping
            SX1278_Write(REG_DIOMAPPING1, 0x00);

            // Map DIO5 to ModeReady / Reference at page 69
            SX1278_Write(REG_DIOMAPPING2, 0x30);
            break;
        case MODEM_LORA:
            SX1278_SetOpmode(RF_OPMODE_SLEEP);
            // Mask LongRangeMode bit and set it to LoRa mode / Reference at page 108
            SX1278_Write(REG_OPMODE, (SX1278_Read(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK) | RFLR_OPMODE_LONGRANGEMODE_ON);

            // Default DIO mapping
            SX1278_Write(REG_DIOMAPPING1, 0x00);

            // Default DIO mapping
            SX1278_Write(REG_DIOMAPPING2, 0x00);
            break;
    }
}

void SX1278_SetOpmode(uint8_t opmode) {
    // Mask Mode bits and set opmode bits accordingl / Reference at page 108
    SX1278_Write(REG_OPMODE, (SX1278_Read(REG_OPMODE) & RF_OPMODE_MASK) | opmode);
}

void SX1278_Write(uint8_t addr, uint8_t data) {
    SPI_ChipEnable(Port_1, CS);

    SPI_TransferByte(addr | 0x80);              // First bit when writing needs to be 1
    SPI_TransferByte(data);

    SPI_ChipDisable(Port_1, CS);
}

void SX1278_WriteBuffer(uint8_t addr, uint8_t* data, uint8_t len) {
    SPI_ChipEnable(Port_1, CS);
    SPI_SendByte(addr | 0x80);                  // First bit when writing needs to be 1

    while (len--)
        SPI_SendByte(*data++);

    SPI_TXReady();                              // Check to see if the TX is finished

    SPI_ChipDisable(Port_1, CS);
}

void SX1278_WriteFIFO(uint8_t *data, uint8_t len) {
    SX1278_WriteBuffer(0, data, len);           // FIFO address / data array / data length
}

uint8_t SX1278_Read(uint8_t addr) {
    SPI_ChipEnable(Port_1, CS);

    SPI_TransferByte(addr & 0x7f);              // First bit when reading needs to be 0
    SPI_TransferByte(0x00);

    uint8_t result = SPI_Buffer;

    SPI_ChipDisable(Port_1, CS);

    return result;
}

void SX1278_ReadBuffer(uint8_t addr, uint8_t* data, uint8_t len) {
    SPI_ChipEnable(Port_1, CS);
    SPI_SendByte(addr & 0x7f);                  // First bit when reading needs to be 0

    while (len--) {
        SPI_TransferByte(0x00);                 // Send an empty byte for data receive
        *data++ = SPI_Buffer;
    }

    SPI_ChipDisable(Port_1, CS);
}

void SX1278_ReadFIFO(uint8_t *data, uint8_t len) {
    SX1278_ReadBuffer(0, data, len);            // FIFO address / data array / data length
}

void SX1278_OnDIO0IRQ() {
    volatile uint8_t irqFlags = 0;

    switch(sx1278.Settings.State) {
        case RF_RX_RUNNING:
            // TimerStop( &RxTimeoutTimer );
            switch(sx1278.Settings.Modem) {
            // Unchecked for FSK
            case MODEM_FSK: {
                if(sx1278.Settings.Fsk.CrcOn == true) {
                    irqFlags = SX1278_Read( REG_IRQFLAGS2 );
                    if((irqFlags & RF_IRQFLAGS2_CRCOK) != RF_IRQFLAGS2_CRCOK) {
                        // Clear Irqs
                        SX1278_Write(REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | RF_IRQFLAGS1_PREAMBLEDETECT | RF_IRQFLAGS1_SYNCADDRESSMATCH);
                        SX1278_Write(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

                        //TimerStop( &RxTimeoutTimer );

                        if(sx1278.Settings.Fsk.RxContinuous == false) {
                            //TimerStop( &RxTimeoutSyncWord );
                            sx1278.Settings.State = RF_IDLE;
                        } else {
                            // Continuous mode restart Rx chain
                            SX1278_Write( REG_RXCONFIG, SX1278_Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                            //TimerStart( &RxTimeoutSyncWord );
                        }

                        if((radio_events != 0) && (radio_events->RxError != 0) ) {
                            radio_events->RxError();
                        }

                        sx1278.Settings.FskPacketHandler.PreambleDetected = false;
                        sx1278.Settings.FskPacketHandler.SyncWordDetected = false;
                        sx1278.Settings.FskPacketHandler.NbBytes = 0;
                        sx1278.Settings.FskPacketHandler.Size = 0;
                        break;
                    }
                }

                // Read received packet size
                if((sx1278.Settings.FskPacketHandler.Size == 0) && (sx1278.Settings.FskPacketHandler.NbBytes == 0)) {
                    if(sx1278.Settings.Fsk.FixLen == false) {
                        SX1278_ReadFIFO((uint8_t*) &sx1278.Settings.FskPacketHandler.Size, 1);
                    } else {
                        sx1278.Settings.FskPacketHandler.Size = SX1278_Read(REG_PAYLOADLENGTH);
                    }
                    SX1278_ReadFIFO(RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes, sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes);
                    sx1278.Settings.FskPacketHandler.NbBytes += (sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes);
                } else {
                    SX1278_ReadFIFO(RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes, sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes);
                    sx1278.Settings.FskPacketHandler.NbBytes += (sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes);
                }

                if(sx1278.Settings.Fsk.RxContinuous == false) {
                    sx1278.Settings.State = RF_IDLE;
                    //TimerStart( &RxTimeoutSyncWord );
                } else {
                    // Continuous mode restart Rx chain
                    SX1278_Write(REG_RXCONFIG, SX1278_Read(REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
                }
                //TimerStop( &RxTimeoutTimer );

                if((radio_events != 0) && (radio_events->RxDone != 0))   {
                    radio_events->RxDone(RxTxBuffer, sx1278.Settings.FskPacketHandler.Size, sx1278.Settings.FskPacketHandler.RssiValue, 0);
                }

                sx1278.Settings.FskPacketHandler.PreambleDetected = false;
                sx1278.Settings.FskPacketHandler.SyncWordDetected = false;
                sx1278.Settings.FskPacketHandler.NbBytes = 0;
                sx1278.Settings.FskPacketHandler.Size = 0;
                }
                break;
            case MODEM_LORA: {
                int8_t snr = 0;

                // Clear Irq
                SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE);

                irqFlags = SX1278_Read(REG_LR_IRQFLAGS);
                if((irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK) == RFLR_IRQFLAGS_PAYLOADCRCERROR) {
                    // Clear Irq
                    SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);

                    if(sx1278.Settings.LoRa.RxContinuous == false)
                        sx1278.Settings.State = RF_IDLE;

                    //TimerStop( &RxTimeoutTimer );

//                    if((radio_events != 0) && (radio_events->RxError != 0))
                    OnRxError();

                    break;
                }

                sx1278.Settings.LoRaPacketHandler.SnrValue = SX1278_Read(REG_LR_PKTSNRVALUE);
                // Check if SNR is negative or positive
                if(sx1278.Settings.LoRaPacketHandler.SnrValue & 0x80) {
                    // Invert and divide by 4
                    snr = ( ( ~sx1278.Settings.LoRaPacketHandler.SnrValue + 1 ) & 0xFF ) >> 2;
                    snr = -snr;
                } else {
                    // Divide by 4
                    snr = ( sx1278.Settings.LoRaPacketHandler.SnrValue & 0xFF ) >> 2;
                }

                int16_t rssi = SX1278_Read(REG_LR_PKTRSSIVALUE);
                if(snr < 0) {
                    if(sx1278.Settings.Channel > RF_MID_BAND_THRESH) {
                        sx1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + (rssi >> 4) + snr;
                    } else {
                        sx1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + (rssi >> 4) + snr;
                    }
                } else {
                    if(sx1278.Settings.Channel > RF_MID_BAND_THRESH) {
                        sx1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + (rssi >> 4);
                    } else {
                        sx1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + (rssi >> 4);
                    }
                }

                sx1278.Settings.LoRaPacketHandler.Size = SX1278_Read(REG_LR_RXNBBYTES);
                SX1278_ReadFIFO(RxTxBuffer, sx1278.Settings.LoRaPacketHandler.Size);

                if(sx1278.Settings.LoRa.RxContinuous == false)
                    sx1278.Settings.State = RF_IDLE;

                //TimerStop( &RxTimeoutTimer );

//                if((radio_events != 0) && (radio_events->RxDone != 0))
                    OnRxDone(RxTxBuffer, sx1278.Settings.LoRaPacketHandler.Size, sx1278.Settings.LoRaPacketHandler.RssiValue, sx1278.Settings.LoRaPacketHandler.SnrValue);
                }
                break;
            default:
                break;
                }
                break;
            case RF_TX_RUNNING:
                //TimerStop( &TxTimeoutTimer );
                switch(sx1278.Settings.Modem) {
                    case MODEM_LORA:
                        // Clear Irq
                        SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE);
                        OnTxDone();
                    case MODEM_FSK:
                    default:
                        sx1278.Settings.State = RF_IDLE;
                        if((radio_events != 0) && (radio_events->TxDone != 0))
                            OnTxDone( );
                        UART_WriteStr("send finished \r\n");

                        break;
                }
                break;
                default:
                    break;
    }
}

void SX1278_OnDIO1IRQ( void )
{
    switch( sx1278.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( sx1278.Settings.Modem )
            {
            // Unchecked for FSK
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if((sx1278.Settings.FskPacketHandler.Size == 0) && (sx1278.Settings.FskPacketHandler.NbBytes == 0))
                {
                    if(sx1278.Settings.Fsk.FixLen == false)
                    {
                        SX1278_ReadFIFO((uint8_t*)&sx1278.Settings.FskPacketHandler.Size, 1);
                    }
                    else
                    {
                        sx1278.Settings.FskPacketHandler.Size = SX1278_Read( REG_PAYLOADLENGTH );
                    }
                }

                if( ( sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes ) > sx1278.Settings.FskPacketHandler.FifoThresh )
                {
                    SX1278_ReadFIFO( ( RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes ), sx1278.Settings.FskPacketHandler.FifoThresh );
                    sx1278.Settings.FskPacketHandler.NbBytes += sx1278.Settings.FskPacketHandler.FifoThresh;
                }
                else
                {
                    SX1278_ReadFIFO( ( RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes ), sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes );
                    sx1278.Settings.FskPacketHandler.NbBytes += ( sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes );
                }
                break;
            case MODEM_LORA:
                // Sync time out
                //TimerStop( &RxTimeoutTimer );
                // Clear Irq
                SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT);

                sx1278.Settings.State = RF_IDLE;
//                if((radio_events != NULL) && (radio_events->RxTimeout != NULL))
//                    radio_events->RxTimeout();
                    OnRxTimeout();
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( sx1278.Settings.Modem )
            {
            // Unchecked for FSK
            case MODEM_FSK:
                // FifoEmpty interrupt
                if((sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes) > sx1278.Settings.FskPacketHandler.ChunkSize)
                {
                    SX1278_WriteFIFO( ( RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes ), sx1278.Settings.FskPacketHandler.ChunkSize );
                    sx1278.Settings.FskPacketHandler.NbBytes += sx1278.Settings.FskPacketHandler.ChunkSize;
                }
                else
                {
                    // Write the last chunk of data
                    SX1278_WriteFIFO( RxTxBuffer + sx1278.Settings.FskPacketHandler.NbBytes, sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes );
                    sx1278.Settings.FskPacketHandler.NbBytes += sx1278.Settings.FskPacketHandler.Size - sx1278.Settings.FskPacketHandler.NbBytes;
                }
                break;
            case MODEM_LORA:
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

void SX1278_OnDIO2IRQ(void) {
    switch( sx1278.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( sx1278.Settings.Modem )
            {
            case MODEM_FSK:
                if( ( sx1278.Settings.FskPacketHandler.PreambleDetected == true ) && ( sx1278.Settings.FskPacketHandler.SyncWordDetected == false ) )
                {
//                    TimerStop( &RxTimeoutSyncWord );

                    sx1278.Settings.FskPacketHandler.SyncWordDetected = true;

                    sx1278.Settings.FskPacketHandler.RssiValue = -( SX1278_Read( REG_RSSIVALUE ) >> 1 );

                    sx1278.Settings.FskPacketHandler.AfcValue = ( int32_t )( double )( ( ( uint16_t )SX1278_Read( REG_AFCMSB ) << 8 ) |
                                                                           ( uint16_t )SX1278_Read( REG_AFCLSB ) ) *
                                                                           ( double )FREQ_STEP;
                    sx1278.Settings.FskPacketHandler.RxGain = ( SX1278_Read( REG_LNA ) >> 5 ) & 0x07;
                }
                break;
            case MODEM_LORA:
                if(sx1278.Settings.LoRa.FreqHopOn == true)
                {
                    // Clear Irq
                    SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

//                    if((radio_events != NULL) && (radio_events->FhssChangeChannel != NULL))
//                        radio_events->FhssChangeChannel((SX1278_Read(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
                    SX1278_ChangeFHSS((SX1278_Read(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( sx1278.Settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if(sx1278.Settings.LoRa.FreqHopOn == true)
                {
                    // Clear Irq
                    SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

//                    if((radio_events != NULL ) && ( radio_events->FhssChangeChannel != NULL))
//                        radio_events->FhssChangeChannel((SX1278_Read(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
                    SX1278_ChangeFHSS((SX1278_Read(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

void SX1278_OnDIO3IRQ(void)
{
    switch(sx1278.Settings.Modem)
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        if((SX1278_Read(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED) == RFLR_IRQFLAGS_CADDETECTED)
        {
            // Clear Irq
            SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE);
            if((radio_events != NULL) && (radio_events->CadDone != NULL))
                radio_events->CadDone(true);
        }
        else
        {
            // Clear Irq
            SX1278_Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE);
            if((radio_events != NULL) && (radio_events->CadDone != NULL))
                radio_events->CadDone(false);
        }
        break;
    default:
        break;
    }
}

void SX1278_OnDIO4IRQ(void)
{
    switch( sx1278.Settings.Modem)
    {
    case MODEM_FSK:
        {
            if(sx1278.Settings.FskPacketHandler.PreambleDetected == false)
                sx1278.Settings.FskPacketHandler.PreambleDetected = true;
        }
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void SX1278_OnDIO5IRQ(void)
{
    switch(sx1278.Settings.Modem)
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}


//void __attribute__ ((interrupt(PORT2_VECTOR))) port_2 (void) {
//    if((P2IFG & BIT2) == BIT2) sx1278_on_dio0irq();
//
//    P2IFG &= (~BIT2); // P2.1 IFG clear
//}
