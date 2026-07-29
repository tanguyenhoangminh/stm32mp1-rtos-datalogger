#ifndef PIR_H
#define PIR_H

#include "stm32mp1xx_hal.h"

void PIR_Init(void);
uint8_t PIR_IsDetected(void);
uint32_t PIR_GetCount(void);
void PIR_EXTI_Callback(void);

#endif
