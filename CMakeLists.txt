cmake_minimum_required(VERSION 3.20)

project(ventblower LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)

set(FREERTOS_SOURCES
    Firmware/FreeRTOS/croutine.c
    Firmware/FreeRTOS/event_groups.c
    Firmware/FreeRTOS/list.c
    Firmware/FreeRTOS/queue.c
    Firmware/FreeRTOS/stream_buffer.c
    Firmware/FreeRTOS/tasks.c
    Firmware/FreeRTOS/timers.c
    Firmware/FreeRTOS/portable/GCC/ARM_CM4F/port.c
    Firmware/FreeRTOS/portable/MemMang/heap_4.c)

add_executable(ventblower
    User/main.c
    User/app/system/taskmanager.c
    User/bsp/bspdebug.c
    User/bsp/adc/adc.c
    User/bsp/blower_vcm/blower_vcm.c
    User/bsp/dvalve/dvalve.c
    User/bsp/eeprom/m24512r.c
    User/bsp/sf06sdk/sensirion_common.c
    User/bsp/sf06sdk/sensirion_i2c.c
    User/bsp/sf06sdk/sensirion_i2c_hal.c
    User/bsp/sf06sdk/sfm_sf06_i2c.c
    User/bsp/sf06sdk/sfm3119.c
    User/bsp/uart/uart.c
    User/bsp/valve/valve.c
    User/module/log/console.c
    User/module/log/consoleRegisted/sysdebug.c
    User/module/log/log.c
    User/module/log/portLog.c
    User/module/log/SEGGER/SEGGER_RTT.c
    User/module/rtos/rtos.c
    User/module/rtos/portrtos.c
    User/tools/controller/pid.c
    User/tools/filter/butterworth/butterworthfilter.c
    User/tools/filter/iir1/iir1.c
    User/tools/filter/numfilter/numfilter.c
    User/tools/ringbuffer/ringbuffer.c
    Firmware/CMSIS/GD/GD32F4xx/Source/system_gd32f4xx.c
    Firmware/CMSIS/GD/GD32F4xx/Source/GCC/startup_gd32f470zetx.s
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_adc.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_dma.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_gpio.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_i2c.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_rcu.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_timer.c
    Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_usart.c
    ${FREERTOS_SOURCES})

target_include_directories(ventblower PRIVATE
    User
    User/app/system
    User/bsp
    User/bsp/adc
    User/bsp/blower_vcm
    User/bsp/dvalve
    User/bsp/eeprom
    User/bsp/sf06sdk
    User/bsp/uart
    User/bsp/valve
    User/module/log
    User/module/log/consoleRegisted
    User/module/log/SEGGER
    User/module/rtos
    User/tools/controller
    User/tools/filter/butterworth
    User/tools/filter/iir1
    User/tools/filter/numfilter
    User/tools/ringbuffer
    Firmware/CMSIS
    Firmware/CMSIS/GD/GD32F4xx/Include
    Firmware/GD32F4xx_standard_peripheral/Include
    Firmware/FreeRTOS/include
    Firmware/FreeRTOS/portable/GCC/ARM_CM4F)

target_compile_definitions(ventblower PRIVATE
    GD32F470)

target_compile_options(ventblower PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wno-unused-parameter)

target_link_options(ventblower PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/Firmware/Ld/GD32F470ZETX_FALSH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/ventblower.map
    -Wl,--print-memory-usage)

add_custom_command(TARGET ventblower POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:ventblower> ${CMAKE_CURRENT_BINARY_DIR}/ventblower.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:ventblower> ${CMAKE_CURRENT_BINARY_DIR}/ventblower.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:ventblower>
    COMMENT "Generating HEX/BIN images and reporting firmware size")
