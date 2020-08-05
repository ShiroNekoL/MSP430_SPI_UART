#include <msp430.h>
#include <stdint.h>
#include "mcu.h"

void MCU_Init() {
  WDTCTL = WDTPW + WDTHOLD;   // Disable watchdog
  BCSCTL1 = CALBC1_16MHZ;     // Set range
  DCOCTL  = CALDCO_16MHZ;     // Set DCO step + modulation

  P1OUT &= 0x00;              // Shut down everything
  P1DIR &= 0x00;
  P2OUT &= 0x00;
  P2DIR &= 0x00;

  P2DIR &= ~BIT0;
//  P2REN |= BIT0;
  P2IE |= BIT0;
  P2IFG &= ~BIT0;
  P2IES &= ~BIT0;

}

void Delay_ms(uint32_t ms) {
  while (ms) {
    __delay_cycles(16 * 998);
  	ms--;
  }
}

void Delay_us(uint32_t us) {
	while (us) {
		__delay_cycles(14); //for 16MHz
		us--;
  }
}

void MCU_MemCpy(uint8_t *dst, const uint8_t *src, uint16_t size) {
    while(size--) *dst++ = *src++;
}
