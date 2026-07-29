#include "PIR.h"

static uint8_t  pir_detected = 0;
static uint32_t pir_count    = 0;

void PIR_Init(void)
{
    pir_detected = 0;
    pir_count    = 0;
}

uint8_t PIR_IsDetected(void)
{
    if(pir_detected)
    {
        pir_detected = 0;  // clear flag
        return 1;
    }
    return 0;
}

uint32_t PIR_GetCount(void)
{
    return pir_count;
}

void PIR_EXTI_Callback(void)
{
    pir_detected = 1;
    pir_count++;
}
