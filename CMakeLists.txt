cmake_minimum_required(VERSION 3.20)

project(nizaw
    VERSION 3.0.0
    DESCRIPTION "Modern Linux System & CLI Framework written in C++20"
    LANGUAGES CXX
)

include(GNUInstallDirs)

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
option(NIZAW_BUILD_CLI "Build the nizaw CLI application" ON)
option(NIZAW_BUILD_AGENT "Build the nizaw agent (remote orchestration)" ON)
option(NIZAW_BUILD_SHARED "Build libnizaw as a shared library" OFF)
option(NIZAW_PORTABLE "Build a portable release with static third-party dependencies" OFF)
option(NIZAW_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(NIZAW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(NIZAW_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(NIZAW_ENABLE_SYSTEMD "Enable systemd/sd-bus backed nizaw::service" ON)
option(NIZAW_ENABLE_LTO "Enable Link-Time Optimization for release builds" ON)

# Transport layer options
option(NIZAW_TRANSPORT_WEBSOCKET "Enable WebSocket transport" OFF)
option(NIZAW_TRANSPORT_MQTT "Enable MQTT transport" OFF)
option(NIZAW_TRANSPORT_GRPC "Enable gRPC transport" OFF)
option(NIZAW_TRANSPORT_ZMQ "Enable ZeroMQ transport" OFF)
option(NIZAW_TRANSPORT_QUIC "Enable QUIC/HTTP3 transport" OFF)

# Portable build validation
if(NIZAW_PORTABLE)
    message(STATUS "========================================")
    message(STATUS "NIZAW_PORTABLE=ON — building self-contained release")
    message(STATUS "========================================")
    if(NIZAW_BUILD_SHARED)
        message(FATAL_ERROR "NIZAW_PORTABLE=ON is incompatible with NIZAW_BUILD_SHARED=ON. Set NIZAW_BUILD_SHARED=OFF.")
    endif()
    if(NIZAW_ENABLE_ASAN OR NIZAW_ENABLE_UBSAN)
        message(WARNING "NIZAW_PORTABLE=ON with sanitizers enabled may produce larger binaries")
    endif()
endif()

# --- Third-party dependencies -----------------------------------------------
# OpenSSL
find_package(OpenSSL REQUIRED)

# Zlib
find_package(ZLIB REQUIRED)

# SQLite3
find_package(SQLite3 REQUIRED)

# Systemd (optional)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD libsystemd sd-bus QUIET)

# Portable dependency resolution
if(NIZAW_PORTABLE)
    message(STATUS "Portable build: verifying static third-party archives")

    # Helper function to locate and verify static library
    function(verify_static_library lib_name hint_names)
        set(search_names "")
        foreach(name ${hint_names})
            list(APPEND search_names "${name}")
        endforeach()

        # Check for static archive
        find_library(${lib_name}_STATIC_LIB
            NAMES ${search_names}
            PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
            PATH_SUFFIXES lib lib64
        )

        if(${lib_name}_STATIC_LIB)
            get_filename_component(lib_ext ${${lib_name}_STATIC_LIB} EXT)
            if(NOT lib_ext STREQUAL ".a")
                message(FATAL_ERROR
                    "Portable build requires static ${lib_name} library, but found shared library: ${${lib_name}_STATIC_LIB}\n"
                    "Please install the static development package (e.g., lib${lib_name}-dev on Debian/Ubuntu)"
                )
            endif()
            message(STATUS "  ${lib_name}: static archive found: ${${lib_name}_STATIC_LIB}")
            set(${lib_name}_FOUND_STATIC TRUE PARENT_SCOPE)
        else()
            message(FATAL_ERROR
                "Portable build requires static ${lib_name} library, but none found.\n"
                "Please install the static development package (e.g., lib${lib_name}-dev on Debian/Ubuntu)"
            )
        endif()
    endfunction()

    # Verify required static libraries
    verify_static_library("ssl" "libssl.a")
    verify_static_library("crypto" "libcrypto.a")
    verify_static_library("z" "libz.a")
    verify_static_library("sqlite3" "libsqlite3.a")

    # Systemd is optional, but if explicitly enabled verify static availability
    if(NIZAW_ENABLE_SYSTEMD)
        verify_static_library("systemd" "libsystemd.a")
    endif()

    message(STATUS "Static dependency verification complete")
endif()

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

# --- Portable build configuration -------------------------------------------
if(NIZAW_PORTABLE)
    # Enable function/data section GC and LTO where safe
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-ffunction-sections -fdata-sections)
        add_link_options(-Wl,--gc-sections)
        if(NIZAW_ENABLE_LTO)
            include(CheckIPOSupported)
            check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
            if(lto_supported)
                set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
                message(STATUS "Portable build: LTO/IPO enabled")
            else()
                message(WARNING "Portable build: LTO/IPO not supported: ${lto_error}")
            endif()
        endif()
    endif()
endif()

# --- nizaw::core -------------------------------------------------------------
add_library(nizaw_core
    src/core/error.cpp
    src/core/log.cpp
    src/core/platform.cpp
    src/core/version.cpp
    src/core/env.cpp
    src/core/write.cpp
)
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_core PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
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
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_system PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
add_library(nizaw::system ALIAS nizaw_system)

target_include_directories(nizaw_system PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_system PUBLIC cxx_std_20)
target_link_libraries(nizaw_system PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::process ----------------------------------------------------------
add_library(nizaw_process
    src/process/info.cpp
)
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_process PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
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
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_filesystem PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
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
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_storage PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
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
    if(NIZAW_PORTABLE)
        # In portable mode, link static systemd explicitly
        target_link_libraries(nizaw_service PUBLIC
            nizaw::core
            nizaw_project_options
            ${SYSTEMD_STATIC_LIB}
        )
        set_target_properties(nizaw_service PROPERTIES
            POSITION_INDEPENDENT_CODE OFF
        )
    else()
        target_link_libraries(nizaw_service PUBLIC nizaw::core nizaw_project_options ${SYSTEMD_LIBRARIES})
    endif()
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
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_network PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
add_library(nizaw::network ALIAS nizaw_network)

target_include_directories(nizaw_network PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_network PUBLIC cxx_std_20)
target_link_libraries(nizaw_network PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::security --------------------------------------------------------
add_library(nizaw_security
    src/security/info.cpp
)
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_security PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
add_library(nizaw::security ALIAS nizaw_security)

target_include_directories(nizaw_security PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_security PUBLIC cxx_std_20)
target_link_libraries(nizaw_security PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::plugin ----------------------------------------------------------
add_library(nizaw_plugin
    src/plugin/info.cpp
)
if(NIZAW_PORTABLE)
    set_target_properties(nizaw_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
    )
endif()
add_library(nizaw::plugin ALIAS nizaw_plugin)

target_include_directories(nizaw_plugin PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nizaw_plugin PUBLIC cxx_std_20)
target_link_libraries(nizaw_plugin PUBLIC nizaw::core nizaw_project_options)

# --- nizaw::remote ----------------------------------------------------------
if(NIZAW_BUILD_AGENT)
    add_library(nizaw_remote
        src/remote/transport.cpp
    )
    add_library(nizaw::remote ALIAS nizaw_remote)

    target_include_directories(nizaw_remote PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(nizaw_remote PUBLIC cxx_std_20)
    target_link_libraries(nizaw_remote PUBLIC nizaw::core nizaw_project_options)

    if(NIZAW_PORTABLE)
        # Link static OpenSSL libraries
        target_link_libraries(nizaw_remote PUBLIC
            ${SSL_STATIC_LIB}
            ${CRYPTO_STATIC_LIB}
        )
        set_target_properties(nizaw_remote PROPERTIES
            POSITION_INDEPENDENT_CODE OFF
        )
    else()
        find_package(OpenSSL REQUIRED)
        target_link_libraries(nizaw_remote PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    endif()

    if(UNIX AND NOT APPLE)
        if(NIZAW_PORTABLE)
            # Link static Zlib and SQLite3
            target_link_libraries(nizaw_remote PUBLIC
                ${Z_STATIC_LIB}
                ${SQLITE3_STATIC_LIB}
            )
        else()
            find_package(ZLIB REQUIRED)
            target_link_libraries(nizaw_remote PUBLIC ZLIB::ZLIB)
            find_library(SQLITE3_LIBRARY sqlite3)
            if(SQLITE3_LIBRARY)
                target_link_libraries(nizaw_remote PUBLIC ${SQLITE3_LIBRARY})
            endif()
        endif()
    endif()

    # --- nizaw::agent -------------------------------------------------------
    add_library(nizaw_agent
        src/agent/agent.cpp
        src/agent/queue.cpp
        src/agent/executor.cpp
    )
    add_library(nizaw::agent ALIAS nizaw_agent)

    target_include_directories(nizaw_agent PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(nizaw_agent PUBLIC cxx_std_20)
    target_link_libraries(nizaw_agent PUBLIC nizaw::core nizaw::system nizaw::process nizaw::filesystem nizaw::remote nizaw_project_options)

    if(UNIX AND NOT APPLE)
        if(NIZAW_PORTABLE)
            target_link_libraries(nizaw_agent PUBLIC
                ${Z_STATIC_LIB}
                ${SQLITE3_STATIC_LIB}
            )
        else()
            target_link_libraries(nizaw_agent PUBLIC ZLIB::ZLIB)
            if(SQLITE3_LIBRARY)
                target_link_libraries(nizaw_agent PUBLIC ${SQLITE3_LIBRARY})
            endif()
        endif()
    endif()
endif()

# --- CLI ---------------------------------------------------------------------
if(NIZAW_BUILD_CLI)
    add_executable(nizaw
        src/cli/main.cpp
        src/cli/cli.cpp
    )
    target_include_directories(nizaw PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(nizaw PUBLIC cxx_std_20)
    target_link_libraries(nizaw PUBLIC nizaw::core nizaw::system nizaw::process nizaw::filesystem nizaw::storage nizaw::network nizaw::service nizaw::security nizaw::plugin nizaw_project_options)

    if(NIZAW_BUILD_AGENT)
        target_link_libraries(nizaw PUBLIC nizaw::agent)
        target_compile_definitions(nizaw PRIVATE NIZAW_BUILD_AGENT=1)
    endif()

    if(NIZAW_PORTABLE)
        set_target_properties(nizaw PROPERTIES
            POSITION_INDEPENDENT_CODE OFF
            LINK_FLAGS "-Wl,--gc-sections"
        )
    endif()
endif()

# --- Examples ----------------------------------------------------------------
if(NIZAW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --- Tests -------------------------------------------------------------------
if(NIZAW_BUILD_TESTS)
    enable_testing()

    # Plugin test fixture remains SHARED — tests dynamic loading mechanism
    add_library(nizaw_test_plugin_fixture SHARED
        tests/plugin/fixtures/test_plugin.cpp
    )
    set_target_properties(nizaw_test_plugin_fixture PROPERTIES
        PREFIX ""
        OUTPUT_NAME "test_plugin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/plugin"
    )
    target_include_directories(nizaw_test_plugin_fixture PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(nizaw_test_plugin_fixture PUBLIC cxx_std_20)

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
        tests/security/test_security.cpp
        tests/plugin/test_plugin.cpp
    )

    if(NIZAW_BUILD_AGENT)
        target_sources(nizaw_core_tests PRIVATE tests/agent/test_agent.cpp)
        target_compile_definitions(nizaw_core_tests PRIVATE NIZAW_BUILD_AGENT=1)
        target_link_libraries(nizaw_core_tests PRIVATE nizaw::agent nizaw::remote)
    endif()
    add_dependencies(nizaw_core_tests nizaw_test_plugin_fixture)
    target_compile_definitions(nizaw_core_tests PRIVATE PLUGIN_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/tests/plugin")
    target_link_libraries(nizaw_core_tests PRIVATE nizaw::core nizaw::system nizaw::process nizaw::filesystem nizaw::storage nizaw::network nizaw::service nizaw::security nizaw::plugin nizaw_project_options)

    add_test(NAME nizaw_core_tests COMMAND nizaw_core_tests)
endif()

# --- Portable dependency variable setup -------------------------------------
# These variables are populated by the verify_static_library function above
if(NIZAW_PORTABLE)
    # Expose static library paths for linking
    set(SSL_STATIC_LIB "${ssl_STATIC_LIB}" CACHE FILEPATH "Static OpenSSL libssl archive")
    set(CRYPTO_STATIC_LIB "${crypto_STATIC_LIB}" CACHE FILEPATH "Static OpenSSL libcrypto archive")
    set(Z_STATIC_LIB "${z_STATIC_LIB}" CACHE FILEPATH "Static Zlib archive")
    set(SQLITE3_STATIC_LIB "${sqlite3_STATIC_LIB}" CACHE FILEPATH "Static SQLite3 archive")
    set(SYSTEMD_STATIC_LIB "${systemd_STATIC_LIB}" CACHE FILEPATH "Static systemd archive")

    message(STATUS "Portable static libraries selected:")
    message(STATUS "  OpenSSL libssl : ${SSL_STATIC_LIB}")
    message(STATUS "  OpenSSL libcrypto: ${CRYPTO_STATIC_LIB}")
    message(STATUS "  Zlib          : ${Z_STATIC_LIB}")
    message(STATUS "  SQLite3       : ${SQLITE3_STATIC_LIB}")
    if(NIZAW_ENABLE_SYSTEMD AND SYSTEMD_FOUND)
        message(STATUS "  systemd       : ${SYSTEMD_STATIC_LIB}")
    elseif(NIZAW_ENABLE_SYSTEMD)
        message(STATUS "  systemd       : disabled (fallback)")
    else()
        message(STATUS "  systemd       : disabled by configuration")
    endif()
    message(STATUS "========================================")
endif()

# --- Verification target ----------------------------------------------------
if(NIZAW_PORTABLE AND NIZAW_BUILD_CLI)
    find_program(READELF_EXECUTABLE readelf)
    find_program(LDD_EXECUTABLE ldd)
    find_proGRAM(OBJDUMP_EXECUTABLE objdump)

    if(READELF_EXECUTABLE OR LDD_EXECUTABLE)
        add_custom_target(verify-portable ALL
            COMMAND ${CMAKE_COMMAND} -E echo "=== Verifying portable binary ==="
            COMMAND ${CMAKE_COMMAND} -E echo "Checking dynamic dependencies of nizaw executable..."
        )

        if(LDD_EXECUTABLE)
            add_custom_command(TARGET verify-portable POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Running ldd check..."
                COMMAND ${LDD_EXECUTABLE} $<TARGET_FILE:nizaw>
                COMMAND ${CMAKE_COMMAND} -E echo "Run scripts/verify-portable.sh for detailed forbidden library check"
            )
        endif()

        if(READELF_EXECUTABLE)
            add_custom_command(TARGET verify-portable POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Running readelf dynamic section check..."
                COMMAND ${READELF_EXECUTABLE} -d $<TARGET_FILE:nizaw>
            )
        endif()

        if(OBJDUMP_EXECUTABLE)
            add_custom_command(TARGET verify-portable POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Running objdump import check..."
                COMMAND ${OBJDUMP_EXECUTABLE} -p $<TARGET_FILE:nizaw>
            )
        endif()

        add_custom_command(TARGET verify-portable POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "=== Portable verification complete ==="
            COMMAND ${CMAKE_COMMAND} -E echo "For automated forbidden library detection, run: scripts/verify-portable.sh"
        )
    else()
        message(WARNING "readelf/ldd/objdump not found — skipping portable verification target")
    endif()
endif()

# --- Installation -----------------------------------------------------------
install(DIRECTORY include/nizaw
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(NIZAW_INSTALL_TARGETS
    nizaw_core
    nizaw_system
    nizaw_process
    nizaw_filesystem
    nizaw_storage
    nizaw_service
    nizaw_network
    nizaw_security
    nizaw_plugin
)

if(NIZAW_BUILD_AGENT)
    list(APPEND NIZAW_INSTALL_TARGETS nizaw_remote nizaw_agent)
endif()

if(NIZAW_BUILD_CLI)
    list(APPEND NIZAW_INSTALL_TARGETS nizaw)
endif()

install(TARGETS ${NIZAW_INSTALL_TARGETS}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NIZAW_PORTABLE)
    message(STATUS "Nizaw ${PROJECT_VERSION} — portable release target available: nizaw")
else()
    message(STATUS "Nizaw ${PROJECT_VERSION} — core + system + process + filesystem + storage + network + service + security + plugin + cli targets available")
endif()