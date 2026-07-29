#ifndef DHT11_H
#define DHT11_H

#include "stm32mp1xx_hal.h"

typedef struct {
    float    temperature;
    float    humidity;
    uint8_t  valid;
} DHT11_Data_t;

/* Thêm dòng khai báo hàm này vào */
void DHT_Init(void);

DHT11_Data_t DHT11_Read(void);

#endif
