#include <stdint.h>

#ifndef MCU_H
#define MCU_H

void MCU_Init();
void Delay_ms(uint32_t ms);
void Delay_us(uint32_t us);
void MCU_MemCpy(uint8_t *dst, const uint8_t *src, uint16_t size);

#endif
