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

    // Init irq interrupts
    P2DIR &= ~(BIT0 | BIT1 | BIT2 /*| BIT3 | BIT4 | BIT5 */); // Set P2.1 SEL as Input
    P2IES &= ~(BIT0 | BIT1 | BIT2 /*| BIT3 | BIT4 | BIT5 */); // Rising endge 0 -> 1
    P2IE  |=  (BIT0 | BIT1 | BIT2 /*| BIT3 | BIT4 | BIT5 */); // Enable interrupt for P2.1
    P2IFG &= ~(BIT0 | BIT1 | BIT2 /*| BIT3 | BIT4 | BIT5 */); // Clear interrupt flag for P2.1
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
