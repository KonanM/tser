cmake_minimum_required(VERSION 3.11)

project(tser_tests CXX)

# ---- Import googletest ----

include(FetchContent)
FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  set(gtest_force_shared_crt ON CACHE INTERNAL "Always use msvcrt.dll" FORCE)
  FetchContent_Populate(googletest)
  add_subdirectory("${googletest_SOURCE_DIR}" "${googletest_BINARY_DIR}")
endif()

# ---- Add root project ----

FetchContent_Declare(tser SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_GetProperties(tser)
if(NOT tser_POPULATED)
  set(tser_INCLUDE_WITHOUT_SYSTEM ON CACHE BOOL "")
  FetchContent_Populate(tser)
  add_subdirectory("${tser_SOURCE_DIR}" "${tser_BINARY_DIR}")
endif()

# ---- Test ----

enable_testing()

add_executable(tser_test SerializeTest.cpp)

target_link_libraries(tser_test PRIVATE gtest_main KonanM::tser)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(tser_test PRIVATE /permissive- /W4 /JMC)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(tser_test PRIVATE
          -Wall
          -Wextra
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-missing-prototypes
          # it would be nice if googletest could fix their clang warnings :-)
          -Wno-undef
          -Wno-global-constructors
          -Wno-language-extension-token
          -Wno-exit-time-destructors
          -Wno-shift-sign-overflow)
endif()

include(GoogleTest)
gtest_discover_tests(tser_test)

option(TSER_OPT_BUILD_SINGLE_HEADER "Build the single header" ON)
if(TSER_OPT_BUILD_SINGLE_HEADER)
  find_package(PythonInterp REQUIRED)
  add_custom_command(TARGET tser_test POST_BUILD
          COMMAND "${PYTHON_EXECUTABLE}" third_party/amalgamate/amalgamate.py -c single_header.json -s .
          WORKING_DIRECTORY "${tser_SOURCE_DIR}")
endif()
