cmake_minimum_required(VERSION 3.15)

project(AirMouse LANGUAGES C ASM)

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

file(GLOB_RECURSE APP_SOURCES "app/*.c")
file(GLOB_RECURSE DRIVER_SOURCES "drivers/*.c")
file(GLOB_RECURSE MIDDLEWARE_SOURCES "middleware/*.c")
file(GLOB_RECURSE HELPER_SOURCES "helpers/*.c")
set(STARTUP_FILE "targets/stm32f407vgtx/startup_stm32f407vgtx.s")

add_executable(${PROJECT_NAME}.elf 
    ${APP_SOURCES} 
    ${DRIVER_SOURCES} 
    ${MIDDLEWARE_SOURCES} 
    ${HELPER_SOURCES} 
    ${STARTUP_FILE}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/drivers/
    # ${CMAKE_SOURCE_DIR}/drivers/stm
    ${CMAKE_SOURCE_DIR}/middleware
    ${CMAKE_SOURCE_DIR}/helpers
    ${CMAKE_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/CMSIS/Device/ST/STM32F4xx/Include
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F407xx
    # USE_HAL_DRIVER
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -O0 -g3 -Wall -ffunction-sections -fdata-sections
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/targets/stm32f407vgtx/STM32F407VGTX_FLASH.ld
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -lm
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)

add_custom_target(flash
    COMMAND openocd -f board/stm32f4discovery.cfg -c "program ${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing the microcontroller..."
)