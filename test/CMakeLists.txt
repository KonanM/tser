cmake_minimum_required(VERSION 3.14)
#FetchContent is only available for version 3.14 and upwards and we need it to get googletest
include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
# Download and unpack googletest at configure time
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
FetchContent_MakeAvailable(googletest)

add_executable(tser_test)
target_sources(tser_test PRIVATE SerializeTest.cpp)
target_link_libraries(tser_test PRIVATE gtest gtest_main tser)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(tser_test PRIVATE /permissive- /W4 /JMC)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tser_test PRIVATE -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes)
    #it would be nice if googletest could fix their clang warnings :-)
    target_compile_options(tser_test PRIVATE -Wno-undef -Wno-global-constructors -Wno-language-extension-token -Wno-exit-time-destructors -Wno-shift-sign-overflow)
endif()

# add our tests automatically to ctest which makes them discoverable by IDE's like MSVS
include(GoogleTest)
gtest_discover_tests(tser_test)

if(TSER_OPT_BUILD_SINGLE_HEADER)
#find python for the amalgamate python script
include(FindPythonInterp)
add_custom_command(TARGET tser_test POST_BUILD
                  COMMAND ${PYTHON_EXECUTABLE} third_party/amalgamate/amalgamate.py -c single_header.json -s .
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
