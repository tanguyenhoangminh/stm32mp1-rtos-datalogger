/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "PIR.h"
#include "openamp.h"
#include "openamp/open_amp.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>
#include "DHT11.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint32_t timestamp;
    uint8_t  pir;
    uint8_t  source;
    uint8_t  reserved[2];
} SensorData_t;

typedef struct {
    uint32_t timestamp;
    float    temperature;
    float    humidity;
    uint8_t  valid; // checksum DHT11 pass/fail
} DhtData_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile float g_temperature = 0;
volatile float g_humidity    = 0;

extern I2C_HandleTypeDef hi2c5;
extern UART_HandleTypeDef huart3;

static struct rpmsg_endpoint logger_ept;
static volatile int rpmsg_ready = 0;

/* Callback nhận message từ A7 (không cần xử lý) */
static int logger_rpmsg_cb(struct rpmsg_endpoint *ept,
                            void *data, size_t len,
                            uint32_t src, void *priv)
{
    (void)ept; (void)data; (void)len;
    (void)src; (void)priv;
    return 0;
}

/* Callback khi A7 unbind */
static void logger_rpmsg_unbind_cb(struct rpmsg_endpoint *ept)
{
    (void)ept;
    rpmsg_ready = 0;
}

/* Callback khi A7 announce service "logger" */
void logger_rpmsg_bind_cb(struct rpmsg_device *rdev,
                           const char *name, uint32_t dest)
{
    (void)rdev;
    OPENAMP_create_endpoint(&logger_ept, name, dest,
                             logger_rpmsg_cb,
                             logger_rpmsg_unbind_cb);
    rpmsg_ready = 1;
}
/* USER CODE END Variables */
/* Definitions for Task_UART */
osThreadId_t Task_UARTHandle;
const osThreadAttr_t Task_UART_attributes = {
  .name = "Task_UART",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Task_Sensor */
osThreadId_t Task_SensorHandle;
const osThreadAttr_t Task_Sensor_attributes = {
  .name = "Task_Sensor",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Task_Logger */
osThreadId_t Task_LoggerHandle;
const osThreadAttr_t Task_Logger_attributes = {
  .name = "Task_Logger",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for Task_OLED */
osThreadId_t Task_OLEDHandle;
const osThreadAttr_t Task_OLED_attributes = {
  .name = "Task_OLED",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Task_DHT */
osThreadId_t Task_DHTHandle;
const osThreadAttr_t Task_DHT_attributes = {
  .name = "Task_DHT",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensorQueue */
osMessageQueueId_t sensorQueueHandle;
const osMessageQueueAttr_t sensorQueue_attributes = {
  .name = "sensorQueue"
};
/* Definitions for displayQueue */
osMessageQueueId_t displayQueueHandle;
const osMessageQueueAttr_t displayQueue_attributes = {
  .name = "displayQueue"
};
/* Definitions for dhtQueue */
osMessageQueueId_t dhtQueueHandle;
const osMessageQueueAttr_t dhtQueue_attributes = {
  .name = "dhtQueue"
};
/* Definitions for I2cMutexHandle */
osMutexId_t I2cMutexHandleHandle;
const osMutexAttr_t I2cMutexHandle_attributes = {
  .name = "I2cMutexHandle"
};
/* Definitions for UartMutexHandle */
osMutexId_t UartMutexHandleHandle;
const osMutexAttr_t UartMutexHandle_attributes = {
  .name = "UartMutexHandle"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartUARTTask(void *argument);
void StartSensorTask(void *argument);
void StartLoggerTask(void *argument);
void StartOledTask(void *argument);
void StartDhtTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of I2cMutexHandle */
  I2cMutexHandleHandle = osMutexNew(&I2cMutexHandle_attributes);

  /* creation of UartMutexHandle */
  UartMutexHandleHandle = osMutexNew(&UartMutexHandle_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sensorQueue */
  sensorQueueHandle = osMessageQueueNew (20, sizeof(SensorData_t), &sensorQueue_attributes);

  /* creation of displayQueue */
  displayQueueHandle = osMessageQueueNew (5, 16, &displayQueue_attributes);

  /* creation of dhtQueue */
  dhtQueueHandle = osMessageQueueNew (5, sizeof(DhtData_t), &dhtQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Task_UART */
  Task_UARTHandle = osThreadNew(StartUARTTask, NULL, &Task_UART_attributes);

  /* creation of Task_Sensor */
  Task_SensorHandle = osThreadNew(StartSensorTask, NULL, &Task_Sensor_attributes);

  /* creation of Task_Logger */
  Task_LoggerHandle = osThreadNew(StartLoggerTask, NULL, &Task_Logger_attributes);

  /* creation of Task_OLED */
  Task_OLEDHandle = osThreadNew(StartOledTask, NULL, &Task_OLED_attributes);

  /* creation of Task_DHT */
  Task_DHTHandle = osThreadNew(StartDhtTask, NULL, &Task_DHT_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartUARTTask */
/**
  * @brief  Function implementing the Task_UART thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartUARTTask */
void StartUARTTask(void *argument)
{
  const char *boot = "\r\nUART Task Started!\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)boot, strlen(boot), 1000);

  char msg[80];
  int len;
  uint32_t loop = 0;
  uint32_t pir_count = 0;  /* thêm counter */

  const char *banner = "\r\n=== STM32MP157D Data Logger ===\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)banner, strlen(banner), 100);

  for (;;)
  {
    osDelay(2000);


    pir_count = PIR_GetCount();

    len = snprintf(msg, sizeof(msg),
                   "[%lu] tick=%lu | PIR=%lu | sQ=%lu | dQ=%lu | dhtQ=%lu\r\n",
                   loop++,
                   (unsigned long)osKernelGetTickCount(),
                   (unsigned long)pir_count,
                   (unsigned long)osMessageQueueGetCount(sensorQueueHandle),
                   (unsigned long)osMessageQueueGetCount(displayQueueHandle),
                   (unsigned long)osMessageQueueGetCount(dhtQueueHandle));

    osMutexAcquire(UartMutexHandleHandle, osWaitForever);
    HAL_UART_Transmit(&huart3, (uint8_t*)msg, len, 100);
    osMutexRelease(UartMutexHandleHandle);
  }
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the Task_Sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  SensorData_t data;
  uint32_t lastTick = 0;
  uint32_t lastDetectTick = 0;
  const uint32_t DEBOUNCE_MS = 500;
  uint8_t ledOn = 0;

  PIR_Init();

  for(;;)
  {
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 100);

    uint32_t now = osKernelGetTickCount();

    if (flags & 0x01)  /* PIR triggered */
    {
      if ((now - lastTick) >= DEBOUNCE_MS)
      {
        lastTick       = now;
        lastDetectTick = now;

        if (PIR_IsDetected())
        {
          data.timestamp   = now;
          data.pir         = 1;
          data.source      = 0;
          data.reserved[0] = 0;
          data.reserved[1] = 0;

          osMessageQueuePut(sensorQueueHandle, &data, 0, 0);
          osMessageQueuePut(displayQueueHandle, &data, 0, 0);

          HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
          ledOn = 1;
        }
      }
    }

    if (ledOn && (now - lastDetectTick) > 3000)
    {
      HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
      ledOn = 0;
    }
  }
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartLoggerTask */
/**
* @brief Function implementing the Task_Logger thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLoggerTask */
void StartLoggerTask(void *argument)
{
  SensorData_t pir_data;
  DhtData_t    dht_data;
  char log_buf[64];
  int  len;

  OPENAMP_create_endpoint(&logger_ept, "rpmsg-datalogger-channel",
                           RPMSG_ADDR_ANY,
                           logger_rpmsg_cb,
                           logger_rpmsg_unbind_cb);

  for (;;)
  {
    OPENAMP_check_for_message();

    /* PIR data */
    if (osMessageQueueGet(sensorQueueHandle, &pir_data, NULL, 0) == osOK)
    {
      len = snprintf(log_buf, sizeof(log_buf),
                     "%lu,PIR,%u\r\n",
                     (unsigned long)pir_data.timestamp,
                     pir_data.pir);
      rpmsg_send(&logger_ept, log_buf, len);
    }

    /* DHT11 data */
    if (osMessageQueueGet(dhtQueueHandle, &dht_data, NULL, 0) == osOK)
    {
      len = snprintf(log_buf, sizeof(log_buf),
                     "%lu,DHT11,%.1f,%.1f\r\n",
                     (unsigned long)dht_data.timestamp,
                     dht_data.temperature,
                     dht_data.humidity);
      rpmsg_send(&logger_ept, log_buf, len);
    }

    osDelay(10);
  }
}

/* USER CODE BEGIN Header_StartOledTask */
/**
* @brief Function implementing the Task_OLED thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOledTask */
void StartOledTask(void *argument)
{
  SensorData_t pir_data;
  char line1[24];
  char line2[24];
  char line3[24];

  float last_temp = 0;
  float last_hum  = 0;

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Data Logger", Font_7x10, White);
  ssd1306_UpdateScreen();

  for (;;)
  {
//
//    if (osMessageQueueGet(dhtQueueHandle, &dht_data, NULL, 0) == osOK)
//    {
//      last_temp = dht_data.temperature;
//      last_hum  = dht_data.humidity;
//    }
	  last_temp = g_temperature;
	  last_hum  = g_humidity;

    /* Nhận PIR data */
    osStatus_t status = osMessageQueueGet(displayQueueHandle,
                                          &pir_data, NULL, 2000);
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);

    /* Line 1: PIR status */
    snprintf(line1, sizeof(line1), "PIR:%s",
             (status == osOK && pir_data.pir) ? "DETECT" : "Clear");
    ssd1306_WriteString(line1, Font_7x10, White);

    /* Line 2: Temperature */
    snprintf(line2, sizeof(line2), "T:%.1fC", last_temp);
    ssd1306_SetCursor(0, 14);
    ssd1306_WriteString(line2, Font_7x10, White);

    /* Line 3: Humidity */
    snprintf(line3, sizeof(line3), "H:%.1f%%", last_hum);
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line3, Font_7x10, White);

    ssd1306_UpdateScreen();
  }
}

/* USER CODE BEGIN Header_StartDhtTask */
/**
* @brief Function implementing the Task_DHT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDhtTask */
void StartDhtTask(void *argument)
{
  /* USER CODE BEGIN StartDhtTask */
  DhtData_t dht;

  osDelay(2000);

  for (;;)
  {


    DHT11_Data_t raw = DHT11_Read();

    if (raw.valid)
    {
      dht.timestamp   = osKernelGetTickCount();
      dht.temperature = raw.temperature;
      dht.humidity    = raw.humidity;
      dht.valid       = 1;

      g_temperature = raw.temperature;
      g_humidity    = raw.humidity;

      osMessageQueuePut(dhtQueueHandle, &dht, 0, 0);

      char msg[64];
      snprintf(msg, sizeof(msg),
               "[DHT] T=%.1fC H=%.1f%%\r\n",
               dht.temperature, dht.humidity);
      osMutexAcquire(UartMutexHandleHandle, osWaitForever);
      HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), 100);
      osMutexRelease(UartMutexHandleHandle);
    }
    else
    {
      char warn[] = "[DHT] Read failed\r\n";
      osMutexAcquire(UartMutexHandleHandle, osWaitForever);
      HAL_UART_Transmit(&huart3, (uint8_t*)warn, strlen(warn), 100);
      osMutexRelease(UartMutexHandleHandle);
    }

    osDelay(2000);
  }
  /* USER CODE END StartDhtTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == PIR_IN_Pin)
  {
    PIR_EXTI_Callback();
    osThreadFlagsSet(Task_SensorHandle, 0x01);
  }
}
/* USER CODE END Application */

