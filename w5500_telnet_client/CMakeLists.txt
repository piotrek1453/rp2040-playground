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
# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location (note this can come from
# environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension
# to work ==
if(WIN32)
  set(USERHOME $ENV{USERPROFILE})
else()
  set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
  include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD
    pico
    CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(w5500_telnet_client C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# W5500 / ioLibrary paths
set(WIZNET_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/ioLibrary_Driver)

set(PORT_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/WIZnet-PICO-C/port)
if(NOT EXISTS ${PORT_DIR}/ioLibrary_Driver/src/wizchip_spi.c)
  message(
    FATAL_ERROR
      "Cannot find WIZnet-PICO-C port files under third_party/WIZnet-PICO-C/port"
  )
endif()

# WIZnet chip and board selection DEVICE_BOARD_NAME=W5500_EVB_PICO selects the
# default SPI0 pin set in wizchip_spi.h: PIN_MISO=16  PIN_CS=17  PIN_SCK=18
# PIN_MOSI=19  SPI_PORT=spi0
add_compile_definitions(_WIZCHIP_=W5500 DEVICE_BOARD_NAME=W5500_EVB_PICO)

# ioLibrary_Driver: WIZnet chip ethernet core (W5500 driver + socket API)
add_library(
  ioLibrary_Driver STATIC
  ${WIZNET_DIR}/Ethernet/wizchip_conf.c ${WIZNET_DIR}/Ethernet/socket.c
  ${WIZNET_DIR}/Ethernet/W5500/w5500.c)
target_include_directories(ioLibrary_Driver PUBLIC ${WIZNET_DIR}/Ethernet
                                                   ${WIZNET_DIR}/Ethernet/W5500)

# ioLibrary_port: RP2040-specific SPI glue + GPIO IRQ (from WIZnet-PICO-C)
add_library(
  ioLibrary_port STATIC ${PORT_DIR}/ioLibrary_Driver/src/wizchip_spi.c
                        ${PORT_DIR}/ioLibrary_Driver/src/wizchip_gpio_irq.c)
target_include_directories(
  ioLibrary_port PUBLIC ${PORT_DIR}/ioLibrary_Driver/inc ${PORT_DIR}
                        ${WIZNET_DIR}/Ethernet)
target_link_libraries(ioLibrary_port PUBLIC pico_stdlib hardware_spi
                                            hardware_dma ioLibrary_Driver)

# ioLibrary_timer: RP2040 1ms repeating timer utility (from WIZnet-PICO-C)
add_library(ioLibrary_timer STATIC ${PORT_DIR}/timer/timer.c)
target_include_directories(ioLibrary_timer PUBLIC ${PORT_DIR}/timer)
target_link_libraries(ioLibrary_timer PRIVATE pico_stdlib)

# Add executable. Default name is the project name, version 0.1

add_executable(w5500_telnet_client src/w5500_telnet_client.c)

pico_set_program_name(w5500_telnet_client "w5500_telnet_client")
pico_set_program_version(w5500_telnet_client "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(w5500_telnet_client 1)
pico_enable_stdio_usb(w5500_telnet_client 0)

# Add the standard library to the build
target_link_libraries(w5500_telnet_client pico_stdlib)

# Add the standard include files to the build
target_include_directories(w5500_telnet_client
                           PRIVATE ${CMAKE_CURRENT_LIST_DIR}
                           inc/)

# Add any user requested libraries
target_link_libraries(w5500_telnet_client hardware_spi hardware_timer
                      hardware_clocks ioLibrary_port ioLibrary_timer)

pico_add_extra_outputs(w5500_telnet_client)
