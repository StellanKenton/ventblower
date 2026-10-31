set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_GCC arm-none-eabi-gcc
    HINTS
        "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin"
        "C:/Program Files/Arm GNU Toolchain arm-none-eabi/bin"
    REQUIRED)

get_filename_component(ARM_TOOLCHAIN_BIN_DIR "${ARM_GCC}" DIRECTORY)
set(CMAKE_C_COMPILER "${ARM_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${ARM_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_OBJCOPY "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "" FORCE)
set(CMAKE_SIZE "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-size${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "" FORCE)

