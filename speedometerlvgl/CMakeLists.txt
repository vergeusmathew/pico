cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include(pico_sdk_import.cmake)

project(speedometer C CXX ASM)
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lvgl lvgl_build)

add_executable(speedometer
 main.cpp
 lcd.cpp
 LCD_Fonts.cpp
 tft_spi.cpp
 gui.cpp
 ds3231.cpp
 Gen1Hz.cpp
 wifi.cpp
)

target_include_directories(speedometer PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}       # Exposes your root workspace folder path
    ${CMAKE_CURRENT_LIST_DIR}/lvgl # Exposes the internal library directory path location
)

# USB serial optimizations
pico_enable_stdio_usb(speedometer 0)  
pico_enable_stdio_uart(speedometer 1) 

#pico_simple_hardware_headers_only_target(resets)
#pico_generate_pio_header(speedometer ${CMAKE_CURRENT_LIST_DIR}/DS1820.pio)
pico_generate_pio_header(speedometer ${CMAKE_CURRENT_LIST_DIR}/Gen1Hz.pio)         

target_link_libraries(speedometer 
	lvgl
	pico_stdlib 
	pico_multicore
	hardware_gpio 
	hardware_spi 
	hardware_dma 
	hardware_rtc 
	hardware_adc 
	hardware_pwm 
	hardware_pio 
	hardware_i2c)
                   
pico_add_extra_outputs(speedometer)
