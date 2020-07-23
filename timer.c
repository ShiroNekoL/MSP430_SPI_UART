#include <timer.h>

void TIMER_Init() {
    WDTCTL = WDTPW | WDTHOLD;                           // Hold watchdog timer

    // Setup DCO / Main clock / Frequency: 1MHz
    DCOCTL = 0;                                         // Reset DCO
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    // Setup SMCLK
    BCSCTL2 &= ~SELS;                                   // Choose DCO as clock source

    // ACLK configs
    BCSCTL1 &= ~XTS;                                    // Low frequency mode selected
    BCSCTL3 |= LFXT1S_0;                                // Choosing 32768 Hz crystal as ACLK
    BCSCTL1 |= DIVA_0;                                  // ACLK divider: 1

    //BCSCTL2 |= SELM_0;                                  // Choose DCO clock as Main Clock
}
