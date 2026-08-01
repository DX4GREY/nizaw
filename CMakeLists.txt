cmake_minimum_required(VERSION 3.20)

project(nizaw
    VERSION 0.0.0
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
option(NIZAW_BUILD_TESTS       "Build unit/integration tests"        ON)
option(NIZAW_BUILD_EXAMPLES    "Build example programs"              OFF)
option(NIZAW_BUILD_CLI         "Build the nizaw CLI application"     ON)
option(NIZAW_BUILD_SHARED      "Build libnizaw as a shared library"  OFF)
option(NIZAW_ENABLE_WARNINGS   "Enable strict compiler warnings"     ON)
option(NIZAW_ENABLE_ASAN       "Enable AddressSanitizer"             OFF)
option(NIZAW_ENABLE_UBSAN      "Enable UndefinedBehaviorSanitizer"   OFF)
option(NIZAW_ENABLE_SYSTEMD    "Enable systemd/sd-bus backed nizaw::service" ON)

# NOTE (Phase 0): No module targets are defined yet. `core` is the first
# target to be added, in Phase 1. This file intentionally stays a thin
# skeleton until there is real source to build — see docs/architecture.md.

message(STATUS "Nizaw ${PROJECT_VERSION} — architecture phase, no buildable targets yet")
