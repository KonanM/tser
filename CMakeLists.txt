cmake_minimum_required(VERSION 3.8)
project(tser VERSION "0.1" LANGUAGES CXX)
set(CMAKE_C_COMPILER cl.exe)
option(TSER_OPT_BUILD_EXAMPLES "Build tser examples" ON)
option(TSER_OPT_BUILD_TESTS "Build and perform tser tests" ON)
option(TSER_OPT_BUILD_SINGLE_HEADER "Build build the single header, when the tests are build" ON)

set(HEADER 
$<INSTALL_INTERFACE:include/tser/base64encoding.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/base64_encoding.hpp>
$<INSTALL_INTERFACE:include/tser/base64encoding.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/varint_encoding.hpp>
$<INSTALL_INTERFACE:include/tser/serialize.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/serialize.hpp>
$<INSTALL_INTERFACE:include/tser/tser.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/tser.hpp>
)

add_library(tser INTERFACE)
target_sources(tser INTERFACE ${HEADER})
target_include_directories(tser INTERFACE $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(tser INTERFACE cxx_std_17)

if(MSVC)
    add_custom_target(tser.header SOURCES ${HEADER})
endif()

if(TSER_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(TSER_OPT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()