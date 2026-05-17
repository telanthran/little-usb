cmake_minimum_required(VERSION 3.15)

project(AirMouse LANGUAGES C ASM)

# Pass -DBOARD=<name> for selecting a custom board
set(BOARD "stm32f407_discovery" CACHE STRING "Target board")

if(BOARD STREQUAL "stm32f407_discovery")
    set(BOARD_SOURCES boards/stm32f407_discovery.c)
    file(GLOB_RECURSE CHIP_DRIVER_SOURCES "drivers/stm/*.c" "drivers/peripherals/*.c")
    set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
    set(STARTUP_FILE "targets/stm32f407vgtx/startup_stm32f407vgtx.s")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/targets/stm32f407vgtx/STM32F407VGTX_FLASH.ld")
    set(BOARD_COMPILE_DEFINITIONS STM32F407xx)
    set(BOARD_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/CMSIS/Device/ST/STM32F4xx/Include
    )
else()
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Valid options: stm32f407_discovery")
endif()

file(GLOB_RECURSE APP_SOURCES        "app/*.c")
file(GLOB_RECURSE MIDDLEWARE_SOURCES "middleware/*.c")
file(GLOB_RECURSE HELPER_SOURCES     "helpers/*.c")

add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${BOARD_SOURCES}
    ${CHIP_DRIVER_SOURCES}
    ${MIDDLEWARE_SOURCES}
    ${HELPER_SOURCES}
    ${STARTUP_FILE}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/boards
    ${CMAKE_SOURCE_DIR}/drivers/
    ${CMAKE_SOURCE_DIR}/middleware
    ${CMAKE_SOURCE_DIR}/helpers
    ${BOARD_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    ${BOARD_COMPILE_DEFINITIONS}
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -O0 -g3 -Wall -ffunction-sections -fdata-sections
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -lm
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)

add_custom_target(flash
    COMMAND openocd -f board/stm32f4discovery.cfg -c "program ${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing microcontroller"
)