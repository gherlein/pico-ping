cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

project(main C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.[ch]")

pico_sdk_init()

add_executable(main    
    ${SOURCES}
)

pico_enable_stdio_usb(main 0)
pico_enable_stdio_uart(main 1)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    include
    sx126x_driver/src
)
add_subdirectory(sx126x_driver)

    
target_link_libraries(main
	pico_stdlib
    pico_unique_id 
	hardware_spi
    sx126x_driver
	# other libs as needed
)

add_custom_target(flash
    COMMAND /bin/bash ../flash main.elf
    DEPENDS main
)

add_custom_target(reset
    COMMAND /bin/bash ../reset
    DEPENDS main
)

pico_add_extra_outputs(main)
