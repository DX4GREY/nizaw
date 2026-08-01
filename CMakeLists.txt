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
option(NIZAW_ENABLE_SYSTEMD "Enable systemd/sd-bus backed nizaw::service" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD libsystemd sd-bus QUIET)
if(SYSTEMD_FOUND)
    message(STATUS "Found systemd/sd-bus support")
elseif(NIZAW_ENABLE_SYSTEMD)
    message(WARNING "NIZAW_ENABLE_SYSTEMD is ON but libsystemd/sd-bus was not found; building service module stub")
endif()

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

# --- nizaw::storage ---------------------------------------------------------
add_library(nizaw_storage
    src/storage/info.cpp
)
add_library(nizaw::storage ALIAS nizaw_storage)

target_include_directories(nizaw_storage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_storage PUBLIC cxx_std_20)
target_link_libraries(nizaw_storage PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::service ---------------------------------------------------------
if(SYSTEMD_FOUND AND NIZAW_ENABLE_SYSTEMD)
    add_library(nizaw_service
        src/service/info.cpp
    )
    target_include_directories(nizaw_service PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${SYSTEMD_INCLUDE_DIRS}
    )
    target_compile_features(nizaw_service PUBLIC cxx_std_20)
    target_compile_options(nizaw_service PUBLIC ${SYSTEMD_CFLAGS_OTHER})
    target_link_libraries(nizaw_service PUBLIC nizaw::core nizaw_project_options ${SYSTEMD_LIBRARIES})
    target_compile_definitions(nizaw_service PUBLIC NIZAW_HAVE_SYSTEMD=1)
else()
    add_library(nizaw_service
        src/service/unsupported.cpp
    )
    target_include_directories(nizaw_service PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(nizaw_service PUBLIC cxx_std_20)
    target_link_libraries(nizaw_service PUBLIC nizaw::core nizaw_project_options)
    target_compile_definitions(nizaw_service PUBLIC NIZAW_HAVE_SYSTEMD=0)
endif()
add_library(nizaw::service ALIAS nizaw_service)

# --- nizaw::network ---------------------------------------------------------
add_library(nizaw_network
    src/network/info.cpp
)
add_library(nizaw::network ALIAS nizaw_network)

target_include_directories(nizaw_network PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_network PUBLIC cxx_std_20)
target_link_libraries(nizaw_network PUBLIC nizaw::core nizaw_project_options)

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
        tests/storage/test_storage.cpp
        tests/network/test_network.cpp
        tests/service/test_service.cpp
    )
    target_link_libraries(nizaw_core_tests PRIVATE nizaw::core nizaw::system nizaw::process nizaw::filesystem nizaw::storage nizaw::network nizaw::service nizaw_project_options)

    add_test(NAME nizaw_core_tests COMMAND nizaw_core_tests)
endif()

message(STATUS "Nizaw ${PROJECT_VERSION} — Phase 7: core + system + process + filesystem + storage + network + service targets available")
