cmake_minimum_required(VERSION 3.8)
project(tser VERSION "0.1" LANGUAGES CXX)

option(TSER_OPT_BUILD_EXAMPLES "Build tser examples" ON)
option(TSER_OPT_BUILD_TESTS "Build and perform tser tests" ON)
option(TSER_OPT_BUILD_SINGLE_HEADER "Build build the single header, when the tests are build" ON)

set(HEADER 
$<INSTALL_INTERFACE:include/tser/Base64Encoding.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/Base64Encoding.hpp>
$<INSTALL_INTERFACE:include/tser/Compare.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/Compare.hpp>
$<INSTALL_INTERFACE:include/tser/Print.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/Print.hpp>
$<INSTALL_INTERFACE:include/tser/Serialize.hpp> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tser/Serialize.hpp>
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