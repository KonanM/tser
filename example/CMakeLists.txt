cmake_minimum_required(VERSION 3.11)

project(tser_examples CXX)

include(FetchContent)
FetchContent_Declare(tser SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_GetProperties(tser)
if(NOT tser_POPULATED)
  FetchContent_Populate(tser)
  add_subdirectory("${tser_SOURCE_DIR}" "${tser_BINARY_DIR}")
endif()

foreach(suffix IN ITEMS "" 2 3)
  add_executable("tser_example${suffix}" "example${suffix}.cpp")

  target_link_libraries("tser_example${suffix}" PRIVATE KonanM::tser)
endforeach()

if(MSVC)
  set(compile_options /W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(compile_options
          -Wall
          -Wextra
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-exit-time-destructors
          # it would be nice if googletest could fix their clang warnings :-)
          -Wno-undef
          -Wno-global-constructors
          -Wno-language-extension-token
          -Wno-exit-time-destructors
          -Wno-shift-sign-overflow)
endif()

target_compile_options(tser_example PRIVATE ${compile_options})
