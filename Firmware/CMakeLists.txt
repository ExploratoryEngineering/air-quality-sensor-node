# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(aqnode)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

EXEC_PROGRAM(reto ARGS -r .. version OUTPUT_VARIABLE aq_version)
EXEC_PROGRAM(reto ARGS -r .. hashname OUTPUT_VARIABLE aq_name)

add_definitions(-DAQ_VERSION="${aq_version}" -DAQ_NAME="${aq_name}")