cmake_minimum_required(VERSION 3.8)

project(tser VERSION 1.0.0 LANGUAGES CXX)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(tser_warning_guard SYSTEM)
if(tser_INCLUDE_WITHOUT_SYSTEM)
  set(tser_warning_guard "")
endif()

# ---- Declare library ----

add_library(tser INTERFACE)
add_library(KonanM::tser ALIAS tser)

target_include_directories(tser
        ${tser_warning_guard}
        INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

target_compile_features(tser INTERFACE cxx_std_17)

# ---- Install ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(tser_directory "tser-${PROJECT_VERSION}")
set(tser_include_directory
        "${CMAKE_INSTALL_INCLUDEDIR}/${tser_directory}")

install(TARGETS tser
        EXPORT tserTargets
        INCLUDES DESTINATION "${tser_include_directory}")

set(tser_install_cmakedir
        "${CMAKE_INSTALL_LIBDIR}/cmake/${tser_directory}")

write_basic_package_version_file(
        tserConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

install(EXPORT tserTargets
        FILE tserConfig.cmake
        NAMESPACE KonanM::
        DESTINATION "${tser_install_cmakedir}")

install(FILES
        "${PROJECT_BINARY_DIR}/tserConfigVersion.cmake"
        DESTINATION "${tser_install_cmakedir}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION "${tser_include_directory}")
