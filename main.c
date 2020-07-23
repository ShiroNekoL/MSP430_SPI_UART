#include <main.h>

int main(void) {
    TIMER_Init();
    GPIO_Init();
    SPI_Init();

    _bis_SR_register(GIE);

//    UC0IE |= UCB0TXIE;
//    _delay_cycles(2000000);
//    P1OUT ^= LED1;

//    _bis_SR_register(LPM3_bits);
	
	return 0;
}
