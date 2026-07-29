################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../OLED/ssd1306.c \
../OLED/ssd1306_fonts.c \
../OLED/ssd1306_tests.c 

OBJS += \
./OLED/ssd1306.o \
./OLED/ssd1306_fonts.o \
./OLED/ssd1306_tests.o 

C_DEPS += \
./OLED/ssd1306.d \
./OLED/ssd1306_fonts.d \
./OLED/ssd1306_tests.d 


# Each subdirectory must supply rules for building sources it contributes
OLED/%.o OLED/%.su OLED/%.cyclo: ../OLED/%.c OLED/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DNO_ATOMIC_64_SUPPORT -DMETAL_INTERNAL -DMETAL_MAX_DEVICE_REGIONS=2 -DVIRTIO_SLAVE_ONLY -DUSE_HAL_DRIVER -DSTM32MP157Dxx -c -I../Core/Inc -I../DHT11 -I../PIR -I../OLED -I../OPENAMP -I../../Middlewares/Third_Party/OpenAMP/open-amp/lib/include -I../../Middlewares/Third_Party/OpenAMP/libmetal/lib/include -I../../Drivers/STM32MP1xx_HAL_Driver/Inc -I../../Drivers/STM32MP1xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Drivers/CMSIS/RTOS2/Include -I../../Drivers/CMSIS/Device/ST/STM32MP1xx/Include -I../../Middlewares/Third_Party/OpenAMP/virtual_driver -I../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-OLED

clean-OLED:
	-$(RM) ./OLED/ssd1306.cyclo ./OLED/ssd1306.d ./OLED/ssd1306.o ./OLED/ssd1306.su ./OLED/ssd1306_fonts.cyclo ./OLED/ssd1306_fonts.d ./OLED/ssd1306_fonts.o ./OLED/ssd1306_fonts.su ./OLED/ssd1306_tests.cyclo ./OLED/ssd1306_tests.d ./OLED/ssd1306_tests.o ./OLED/ssd1306_tests.su

.PHONY: clean-OLED

