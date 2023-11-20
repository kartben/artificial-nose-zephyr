#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE
        src/main.c
        src/sensor_acquisition.c
        src/inference.cpp
        )

target_sources_ifdef(CONFIG_DISPLAY app PRIVATE
        src/gui.c
        src/gui_screens/live_chart.c
        )

target_sources_ifdef(CONFIG_APP_MQTT app PRIVATE
        src/mqtt.c
        )

include(edge-impulse-sdk/cmake/utils.cmake)

# Add the Edge Impulse SDK
add_subdirectory(edge-impulse-sdk/cmake/zephyr)

# Add model sources
target_sources(app PRIVATE
    tflite-model/tflite_learn_143_compiled.cpp
    )

# Edge impulse SDK include directories
set(INCLUDES
    .
    include
    tflite-model
    model-parameters
    )
include_directories(${INCLUDES})
