cmake_minimum_required(VERSION 3.20)
project(nizaw_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This examples/ directory is designed to be built inside the nizaw repository
# tree. The top-level CMakeLists.txt exposes the following module targets:
#
#   nizaw::core        - error handling, logging, platform detection, env, version
#   nizaw::system      - host/kernel/uptime/system information
#   nizaw::process     - process enumeration and inspection via /proc
#   nizaw::filesystem  - disk usage and filesystem metadata
#   nizaw::storage     - block device enumeration and metadata
#   nizaw::network     - interface enumeration and address reporting
#   nizaw::service     - service listing and status introspection
#   nizaw::security    - UID/GID/group and capability reporting
#   nizaw::plugin      - plugin discovery and loading
#
# To build the examples, configure the top-level project with:
#
#   cmake -S . -B build -DNIZAW_BUILD_EXAMPLES=ON
#   cmake --build build --parallel --target nizaw_example
#
# The example links against the modules it uses. The core module is pulled in
# transitively by every other module, but is listed explicitly for clarity.

add_executable(nizaw_example main.cpp)

target_link_libraries(nizaw_example PRIVATE
    nizaw::core
    nizaw::system
    nizaw::process
    nizaw::filesystem
    nizaw::network
    nizaw::security
    nizaw::storage
)

# --- Plugin example ----------------------------------------------------------
# Membangun plugin contoh sebagai shared library (.so).
#
# PREFIX ""  : Menghilangkan prefix "lib" dari nama file output.
#              Nizaw mencari file dengan ekstensi .so di direktori plugin.
# OUTPUT_NAME: Nama file output menjadi "hello_plugin.so".
add_library(nizaw_example_hello_plugin SHARED
    plugins/hello_plugin.cpp
)
set_target_properties(nizaw_example_hello_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hello_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
)
target_include_directories(nizaw_example_hello_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_features(nizaw_example_hello_plugin PUBLIC cxx_std_20)

# --- Plugin loader example ---------------------------------------------------
# Program yang memuat dan menampilkan informasi plugin dari sebuah direktori.
add_executable(nizaw_example_plugin_loader
    plugins/plugin_loader.cpp
)
target_include_directories(nizaw_example_plugin_loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_features(nizaw_example_plugin_loader PUBLIC cxx_std_20)
target_link_libraries(nizaw_example_plugin_loader PRIVATE
    nizaw::plugin
    nizaw::core
    ${CMAKE_DL_LIBS}
)

# Pastikan plugin dibangun sebelum loader dijalankan.
add_dependencies(nizaw_example_plugin_loader nizaw_example_hello_plugin)
