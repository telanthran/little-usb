# Tell CMake we are cross-compiling
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set the C and Assembly compilers
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# Prevent CMake from trying to compile standard C++ test programs during setup
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)