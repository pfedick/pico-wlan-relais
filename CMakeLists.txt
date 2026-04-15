# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_ENABLE_CXX_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# .env Datei einlesen
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" ENV_LINES)
    foreach(LINE ${ENV_LINES})
        if(LINE MATCHES "^([^=]+)=(.*)$")
            set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
        endif()
    endforeach()
else()
    message(WARNING ".env nicht gefunden!")
endif()


project(wlanrelais C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(wlanrelais
    src/main.cpp
    src/resources.cpp


    pico-pplib/src/core/string.cpp
    pico-pplib/src/core/stringfunctions.cpp
    pico-pplib/src/core/array.cpp
    pico-pplib/src/core/widestringrepresentation.cpp
    pico-pplib/src/core/exceptions.cpp
    pico-pplib/src/core/bytearrayptr.cpp
    pico-pplib/src/core/bytearray.cpp
    pico-pplib/src/core/functions.cpp
    pico-pplib/src/core/pfpfile.cpp
    
    pico-pplib/src/grafix/drawable.cpp
    pico-pplib/src/grafix/grafix.cpp
    pico-pplib/src/grafix/point.cpp
    pico-pplib/src/grafix/color.cpp
    pico-pplib/src/grafix/rect.cpp
    pico-pplib/src/grafix/size.cpp
    pico-pplib/src/grafix/fonts.cpp
    pico-pplib/src/grafix/font6.cpp

    pico-pplib/src/hardware/lightstrip.cpp
    pico-pplib/src/hardware/oled.cpp

)

pico_set_program_name(wlanrelais "wlanrelais")
pico_set_program_version(wlanrelais "0.1")

# Generate PIO header
pico_generate_pio_header(wlanrelais ${CMAKE_CURRENT_LIST_DIR}/pico-pplib/src/hardware/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(wlanrelais 0)
pico_enable_stdio_usb(wlanrelais 0)

# Add the standard library to the build
target_link_libraries(wlanrelais
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(wlanrelais PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/resources
        ${CMAKE_CURRENT_LIST_DIR}/pico-pplib/include
)
# Compile Definitions
target_compile_definitions(wlanrelais PRIVATE
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASSWORD="${WIFI_PASSWORD}"
)

target_include_directories(wlanrelais PRIVATE include)

# Add any user requested libraries
target_link_libraries(wlanrelais
        pico_stdlib
        pico_rand
        hardware_i2c
        hardware_pio
        hardware_dma
        hardware_watchdog
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        
        )

pico_add_extra_outputs(wlanrelais)

