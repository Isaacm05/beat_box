cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(protomatter_test_project)

pico_sdk_init()

add_executable(main
    main.c
    core.c
    rp2040.c
    arduino_compat.c
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(main pico_stdlib hardware_gpio hardware_irq hardware_pwm hardware_timer)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
