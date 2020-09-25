# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(aqnode)

# For nanoPB
find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ../common/protobuf/aq.proto)
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ../common/protobuf/config.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(generate_proto_sources DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources} ${PROTO_SRCS} ${PROTO_HDRS})

set(CURDIR ${CMAKE_CURRENT_LIST_DIR} )

EXEC_PROGRAM(reto ARGS -r ${CURDIR} version OUTPUT_VARIABLE aq_version)
EXEC_PROGRAM(reto ARGS -r ${CURDIR} hashname OUTPUT_VARIABLE aq_name)

add_definitions(-DAQ_VERSION="${aq_version}" -DAQ_NAME="${aq_name}")

message( "Current dir: ${CURDIR}" )
message( "Version is ${aq_version}" )
message( "Name is ${aq_name}" )

