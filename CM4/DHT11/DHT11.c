#include "DHT11.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"



#define DHT_DIR_OUT()  do { GPIOF->MODER &= ~(3U << (9 * 2)); GPIOF->MODER |= (1U << (9 * 2)); } while(0)

#define DHT_DIR_IN()   do { GPIOF->MODER &= ~(3U << (9 * 2)); } while(0)

/* Ghi mức HIGH / LOW nhanh qua thanh ghi BSRR */
#define DHT_OUT_HIGH() do { GPIOF->BSRR = GPIO_PIN_9; } while(0)
#define DHT_OUT_LOW()  do { GPIOF->BSRR = (uint32_t)GPIO_PIN_9 << 16U; } while(0)

/* Đọc trạng thái chân qua thanh ghi IDR */
#define DHT_READ_PIN() ((GPIOF->IDR & GPIO_PIN_9) != 0)



static inline void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetMCUFreq() / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

DHT11_Data_t DHT11_Read(void)
{
    DHT11_Data_t result = {0, 0, 0};
    uint8_t data[5] = {0};
    uint32_t timeout;


    DHT_DIR_OUT();
    DHT_OUT_LOW();
    osDelay(20);

    taskENTER_CRITICAL();


    DHT_DIR_IN();
    delay_us(30);

    /*  Đợi tín hiệu phản hồi từ DHT11 (LOW -> HIGH -> LOW) */
    timeout = 10000;
    while(!DHT_READ_PIN() && timeout) timeout--; // Đợi hết LOW
    if(timeout == 0) { taskEXIT_CRITICAL(); return result; }

    timeout = 10000;
    while(DHT_READ_PIN() && timeout) timeout--; // Đợi hết HIGH
    if(timeout == 0) { taskEXIT_CRITICAL(); return result; }

    /* 4. Đọc 40 bit */
    for (int i = 0; i < 40; i++)
    {
        timeout = 10000;
        while(!DHT_READ_PIN() && timeout) timeout--; // Chờ hết LOW đầu bit
        if(timeout == 0) { taskEXIT_CRITICAL(); return result; }

        delay_us(40); // Nhảy qua 40us để lấy mẫu mức điện áp

        if (DHT_READ_PIN()) {
            data[i / 8] |= (1 << (7 - (i % 8)));

            // Nếu bit là 1 (xung HIGH dài 70us), chờ cho rớt xuống LOW lại
            timeout = 10000;
            while(DHT_READ_PIN() && timeout) timeout--;
            if(timeout == 0) { taskEXIT_CRITICAL(); return result; }
        }
    }

    taskEXIT_CRITICAL();

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        result.humidity    = data[0];
        result.temperature = data[2];
        result.valid       = 1;
    }

    return result;
}
