cmake_minimum_required(VERSION 3.20)

project(nizaw
    VERSION 0.1.0
    DESCRIPTION "Modern Linux System & CLI Framework written in C++20"
    LANGUAGES CXX
)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# --- Project options -------------------------------------------------------
option(NIZAW_BUILD_TESTS "Build unit/integration tests" ON)
option(NIZAW_BUILD_EXAMPLES "Build example programs" OFF)
option(NIZAW_BUILD_CLI "Build the nizaw CLI application" OFF)
option(NIZAW_BUILD_SHARED "Build libnizaw as a shared library" OFF)
option(NIZAW_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(NIZAW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(NIZAW_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(NIZAW_ENABLE_SYSTEMD "Enable systemd/sd-bus backed nizaw::service" OFF)

# --- Compiler warnings / sanitizers -----------------------------------------
add_library(nizaw_project_options INTERFACE)

if(NIZAW_ENABLE_WARNINGS)
    target_compile_options(nizaw_project_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow
    )
endif()

if(NIZAW_ENABLE_ASAN)
    target_compile_options(nizaw_project_options INTERFACE -fsanitize=address)
    target_link_options(nizaw_project_options INTERFACE -fsanitize=address)
endif()

if(NIZAW_ENABLE_UBSAN)
    target_compile_options(nizaw_project_options INTERFACE -fsanitize=undefined)
    target_link_options(nizaw_project_options INTERFACE -fsanitize=undefined)
endif()

# --- nizaw::core -------------------------------------------------------------
add_library(nizaw_core
    src/core/error.cpp
    src/core/log.cpp
    src/core/platform.cpp
    src/core/version.cpp
    src/core/env.cpp
)
add_library(nizaw::core ALIAS nizaw_core)

target_include_directories(nizaw_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_core PUBLIC cxx_std_20)
target_link_libraries(nizaw_core PUBLIC nizaw_project_options)

# --- nizaw::system -----------------------------------------------------------
add_library(nizaw_system
    src/system/info.cpp
)
add_library(nizaw::system ALIAS nizaw_system)

target_include_directories(nizaw_system PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_system PUBLIC cxx_std_20)
target_link_libraries(nizaw_system PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::process ---------------------------------------------------------
add_library(nizaw_process
    src/process/info.cpp
)
add_library(nizaw::process ALIAS nizaw_process)

target_include_directories(nizaw_process PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_process PUBLIC cxx_std_20)
target_link_libraries(nizaw_process PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::filesystem ------------------------------------------------------
add_library(nizaw_filesystem
    src/filesystem/info.cpp
)
add_library(nizaw::filesystem ALIAS nizaw_filesystem)

target_include_directories(nizaw_filesystem PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_filesystem PUBLIC cxx_std_20)
target_link_libraries(nizaw_filesystem PUBLIC nizaw::core nizaw_project_options)

# --- Tests -------------------------------------------------------------------
if(NIZAW_BUILD_TESTS)
    enable_testing()

    add_executable(nizaw_core_tests
        tests/support/main.cpp
        tests/core/test_result.cpp
        tests/core/test_env.cpp
        tests/core/test_platform.cpp
        tests/core/test_version.cpp
        tests/core/test_log.cpp
        tests/system/test_system.cpp
        tests/process/test_process.cpp
        tests/filesystem/test_filesystem.cpp
    )
    target_link_libraries(nizaw_core_tests PRIVATE nizaw::core nizaw::system nizaw::process nizaw::filesystem nizaw_project_options)

    add_test(NAME nizaw_core_tests COMMAND nizaw_core_tests)
endif()

message(STATUS "Nizaw ${PROJECT_VERSION} — Phase 2: core + system targets available")
