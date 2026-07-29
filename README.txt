================================================================
PROJECT: RTOS-based Data Logger with PIR + DHT11 on STM32MP157D-DK1
Architecture: AMP (Asymmetric Multi-Processing) — M4 FreeRTOS + A7 Linux
================================================================

================================================================
1. SYSTEM ARCHITECTURE OVERVIEW
================================================================

┌─────────────────────────────────────────────────────────────┐
│                     A7 Core (Linux / OpenSTLinux)           │
│  - Owns SDMMC1 (SD card)                                   │
│  - logger_daemon: receives RPMSG → writes CSV to SD card   │
│  - RAM buffer (/tmp/) → periodic flush to SD (/var/log/)   │
└────────────────────────┬────────────────────────────────────┘
                         │ OpenAMP / RPMSG channel
                         │ "rpmsg-datalogger-channel"
┌────────────────────────▼────────────────────────────────────┐
│                     M4 Core (FreeRTOS / CMSIS-RTOS V2)      │
│                                                             │
│  Task_Sensor   (High,        512B) — PIR EXTI8 → queue     │
│  Task_DHT      (Normal,      256B) — DHT11 polling every 2s│
│  Task_Logger   (BelowNormal,1024B) — queue → RPMSG → A7   │
│  Task_OLED     (Low,         512B) — displayQueue → OLED   │
│  Task_UART     (AboveNormal, 256B) — debug print every 2s  │
│                                                             │
│  Queues:                                                    │
│  sensorQueue  (20 items, SensorData_t) — PIR → Logger      │
│  displayQueue ( 5 items, SensorData_t) — PIR → OLED        │
│  dhtQueue     ( 5 items, DhtData_t)    — DHT → Logger      │
│                                                             │
│  Globals: g_temperature, g_humidity — DHT → OLED directly  │
└─────────────────────────────────────────────────────────────┘

================================================================
2. HARDWARE COMPONENTS
================================================================
Device              | Model                    | Interface           | Voltage
--------------------|--------------------------|---------------------|------------------
Development Kit     | STM32MP157D-DK1          | ST-LINK/SWD         | 5V
PIR Sensor          | AM312-Mini               | GPIO Digital/EXTI   | 5V (output 3.3V)
Temperature Sensor  | DHT11                    | 1-Wire (PF9)        | 3.3V
OLED Display        | SSD1306 0.96" 128x64     | I2C5 (PA11/PA12)    | 3.3V
Status LED          | LD7 (onboard)            | GPIO Output (PH7)   | 3.3V
Debug UART          | CP2102 USB-TTL           | USART3 (PB10/PB12)  | 3.3V
SD Card             | microSD (onboard CN15)   | SDMMC1 4-bit wide   | 3.3V

================================================================
3. WIRING DIAGRAM (CN2 Header)
================================================================
Device          | Device Pin    | CN2 Pin | STM32 | Notes
----------------|---------------|---------|-------|----------------------------
PIR AM312-Mini    | VCC           | Pin 2   | -     | 5V
PIR AM312-Mini    | GND           | Pin 6   | -     | GND
PIR AM312-Mini    | OUT           | Pin 11  | PG8   | EXTI8, Rising Edge
OLED SSD1306    | VCC           | Pin 1   | -     | 3.3V
OLED SSD1306    | GND           | Pin 9   | -     | GND
OLED SSD1306    | SDA           | Pin 3   | PA12  | I2C5_SDA, Fast 400kHz
OLED SSD1306    | SCL           | Pin 5   | PA11  | I2C5_SCL, Fast 400kHz
DHT11           | VCC           | Pin 17  | -     | 3.3V
DHT11           | GND           | Pin 20  | -     | GND
DHT11           | DATA          | Pin 19  | PF9   | GPIO Output Open Drain
CP2102          | RX            | Pin 8   | PB10  | USART3_TX
CP2102          | TX            | Pin 10  | PB12  | USART3_RX
CP2102          | GND           | Pin 6   | -     | GND
LED LD7         | -             | -       | PH7   | Onboard LED

================================================================
4. CUBEMX CONFIGURATION
================================================================
• PG8  → GPIO_EXTI8     : Rising Edge, Pull-down              (PIR input)
• PA11 → I2C5_SCL       : Fast Mode 400kHz, assign M4         (OLED SCL)
• PA12 → I2C5_SDA       : Fast Mode 400kHz, assign M4         (OLED SDA)
• PF9  → GPIO_Output    : Open Drain, assign M4, high speed   (DHT11 DATA)
• PH7  → GPIO_Output    : Push-Pull, Low speed                (Status LED)
• PB10 → USART3_TX      : Asynchronous, 115200 baud, assign M4(Debug TX)
• PB12 → USART3_RX      : Asynchronous, 115200 baud, assign M4(Debug RX)
• SDMMC1               : SD 4-bit Wide bus, assign A7NS only  (SD Card)
• IPCC                 : Enable                               (OpenAMP)
• HSEM                 : Enable                               (HW Semaphore)

Middleware:
• OpenAMP  : Activated, RPMSG_REMOTE
• FreeRTOS : CMSIS_V2, Heap 30KB

================================================================
5. CSV LOG FILE FORMAT
================================================================
RAM file : /tmp/datalog.csv       (continuous write, SD-safe)
SD file  : /var/log/datalog.csv   (flushed every 10 entries or 60 seconds)

Format:
real_time,tick_ms,sensor,value1,value2
# Session started: 2026-07-28 18:00:00
2026-07-28 18:00:15,25645,PIR,1,
2026-07-28 18:00:16,26500,DHT11,30.0,72.0

================================================================
6. DATA FLOW
================================================================

PIR AM312-Mini
    │ EXTI Rising Edge (interrupt-driven)
    ▼
Task_Sensor ──► sensorQueue ──► Task_Logger ──► RPMSG ──► A7 daemon ──► CSV
    │
    └──────► displayQueue ──► Task_OLED ──► SSD1306

DHT11
    │ 1-Wire polling every 2s
    ▼
Task_DHT ──► dhtQueue ──► Task_Logger ──► RPMSG ──► A7 daemon ──► CSV
    │
    └──► g_temperature / g_humidity (global) ──► Task_OLED ──► SSD1306

================================================================
7. OLED DISPLAY LAYOUT (128x64)
================================================================
┌─────────────────┐
│ PIR: DETECT     │ ← Line 1 (y=0)  — Motion detection status
│ T: 30.0C        │ ← Line 2 (y=14) — Temperature from DHT11
│ H: 72.0%        │ ← Line 3 (y=28) — Humidity from DHT11
└─────────────────┘

================================================================
8. UART DEBUG OUTPUT (115200 baud, CP2102 COM5)
================================================================
=== STM32MP157D Data Logger ===
[0]  tick=2004  | PIR=0 | sQ=0 | dQ=0 | dhtQ=0
[DHT] T=30.0C H=72.0%
[1]  tick=4008  | PIR=1 | sQ=0 | dQ=0 | dhtQ=0
[DHT] T=30.0C H=71.0%

Fields:
• tick    — FreeRTOS kernel tick count (ms since M4 boot)
• PIR     — total PIR detection count since boot
• sQ      — sensorQueue pending items (0 = healthy, no backlog)
• dQ      — displayQueue pending items
• dhtQ    — dhtQueue pending items

================================================================
9. SYSTEM STARTUP PROCEDURE
================================================================
# On Linux terminal (Tera Term / PuTTY COM9, 115200 baud):

# Step 1 — Copy firmware to board (first time or after new build)
# From Windows PC:
scp "CM4\Debug\SPI_SD_CM4.elf" root@192.168.7.1:/lib/firmware/

# Step 2 — Start M4 core
echo SPI_SD_CM4.elf > /sys/class/remoteproc/remoteproc0/firmware
echo start > /sys/class/remoteproc/remoteproc0/state

# Step 3 — Run logger daemon
./logger_daemon

# Step 4 — Monitor log in real-time
tail -f /tmp/datalog.csv       # RAM buffer (updates continuously)
tail -f /var/log/datalog.csv   # SD card   (updates every 20 entries)

# Step 5 — Stop system
echo stop > /sys/class/remoteproc/remoteproc0/state

================================================================
10. ERROR HANDLING
================================================================
Scenario             | Detection                | Response
---------------------|--------------------------|---------------------------
SD nearly full       | statvfs() < 10MB free    | WARNING printed to daemon
SD flush failed      | system("cp ...") != 0    | Data safe in RAM /tmp/
RPMSG send fail      | rpmsg_send() < 0         | Error logged via UART3
DHT read fail        | checksum mismatch        | "[DHT] Read failed" via UART
No RPMSG recipient   | kernel log warning       | Start daemon before sensors
M4 crash/hang        | UART output stops        | echo stop → echo start M4

