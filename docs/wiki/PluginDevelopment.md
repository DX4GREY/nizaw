# Plugin Development

This guide explains how to create, build, and load Nizaw plugins. Plugins are shared objects (`.so` files) that extend Nizaw with custom commands without modifying the core library.

## Table of Contents

- [Overview](#overview)
- [Plugin API Contract](#plugin-api-contract)
- [Creating a Plugin](#creating-a-plugin)
- [Required Exports](#required-exports)
- [Optional Exports](#optional-exports)
- [Building a Plugin](#building-a-plugin)
- [Loading Plugins](#loading-plugins)
- [Complete Example](#complete-example)
- [Best Practices](#best-practices)

## Overview

The Nizaw plugin system allows you to:

- Add custom commands to the Nizaw CLI
- Extend functionality without recompiling the core library
- Share functionality across multiple Nizaw installations
- Isolate experimental features from the main codebase

Plugins are loaded dynamically from `.so` files and discovered from a directory. Each plugin exports a descriptor and lifecycle functions that Nizaw calls to integrate the plugin.

## Plugin API Contract

All plugins must adhere to the following contract:

1. **Plugin Descriptor**: A `nizaw::plugin::PluginDescriptor` struct exported as `nizaw_plugin_descriptor`
2. **API Version**: Must match `nizaw-plugin-v1`
3. **Lifecycle Functions**: Optional `init` and `cleanup` functions
4. **Command Execution**: An optional `execute` function that runs plugin commands
5. **Metadata Functions**: Optional functions to expose version and command list dynamically

### Plugin Descriptor

```cpp
extern "C" const nizaw::plugin::PluginDescriptor nizaw_plugin_descriptor = {
    "my-plugin",           // name
    "1.0.0",              // version
    "Plugin description", // description
    "nizaw-plugin-v1",    // api_version (must match PluginApiVersion)
    "cmd1,cmd2,cmd3"      // commands (comma-separated list)
};
```

## Creating a Plugin

### Step 1: Define Plugin State

Plugins can maintain internal state. Since plugins are loaded as shared libraries, state is typically stored in a global variable:

```cpp
namespace {
    struct PluginState {
        bool initialized = false;
        int load_count = 0;
        // Add your custom state here
    };
    
    PluginState g_state;
}
```

### Step 2: Implement Helper Functions

Create helper functions for logging or other shared functionality:

```cpp
#include <cstdio>

void log_info(const std::string& message) {
    std::fprintf(stderr, "[my-plugin] INFO: %s\n", message.c_str());
}

void log_error(const std::string& message) {
    std::fprintf(stderr, "[my-plugin] ERROR: %s\n", message.c_str());
}
```

### Step 3: Implement Command Functions

Each command your plugin supports should have a corresponding function:

```cpp
extern "C" const char* my_command(const char* args) {
    static std::string result;
    
    // Your command logic here
    result = "Command executed with args: " + std::string(args ? args : "(none)");
    
    log_info("Command 'my_command' executed");
    return result.c_str();
}
```

**Important**: Returned strings must remain valid for the lifetime of the plugin. Use `static std::string` or manage memory carefully.

### Step 4: Export Required Functions

## Required Exports

### Plugin Descriptor (Required)

```cpp
extern "C" const nizaw::plugin::PluginDescriptor nizaw_plugin_descriptor;
```

This must be defined at the global scope with `extern "C"` linkage to prevent name mangling.

### Init Function (Required)

```cpp
extern "C" int nizaw_plugin_init() {
    if (g_state.initialized) {
        log_info("Plugin already initialized, skipping");
        return 0;
    }
    
    g_state.initialized = true;
    g_state.load_count++;
    
    log_info("Plugin initialized (load #" + std::to_string(g_state.load_count) + ")");
    return 0; // Return 0 on success, non-zero on failure
}
```

Called when the plugin is first loaded. Return 0 for success.

## Optional Exports

### Cleanup Function (Optional)

```cpp
extern "C" void nizaw_plugin_cleanup() {
    if (g_state.initialized) {
        log_info("Plugin cleaned up");
        g_state.initialized = false;
    }
}
```

Called when the plugin is about to be unloaded.

### Execute Function (Optional)

```cpp
extern "C" const char* nizaw_plugin_execute(const char* command, const char* args) {
    if (!command) {
        return "Error: no command provided";
    }
    
    const std::string_view cmd(command);
    
    if (cmd == "my_command") {
        return my_command(args);
    }
    
    static std::string error_result;
    error_result = "Error: unknown command: " + std::string(command);
    log_error("Unknown command: '" + std::string(command) + "'");
    return error_result.c_str();
}
```

Called to execute a command. The `command` parameter is the command name, and `args` is the argument string.

### Version Function (Optional)

```cpp
extern "C" const char* nizaw_plugin_version() {
    return "1.0.0";
}
```

Returns the plugin version string.

### Commands Function (Optional)

```cpp
extern "C" const char* nizaw_plugin_commands() {
    return "cmd1,cmd2,cmd3";
}
```

Returns a comma-separated list of supported commands.

## Building a Plugin

### Using CMake

Create a `CMakeLists.txt` for your plugin:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(my_plugin SHARED my_plugin.cpp)

target_include_directories(my_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/path/to/nizaw/include
)

# Set output name without "lib" prefix if desired
set_target_properties(my_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "my_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
)
```

Build the plugin:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
```

### Manual Compilation

Compile directly with g++:

```bash
g++ -std=c++20 -fPIC -shared -I../../include \
    -o my_plugin.so my_plugin.cpp
```

## Loading Plugins

### Using the Registry API

```cpp
#include <iostream>
#include <nizaw/plugin.hpp>

int main() {
    nizaw::plugin::Registry registry;
    
    // Load all plugins from a directory
    const auto result = registry.load_directory("./plugins");
    if (!result) {
        std::cerr << "Failed to load plugins: " << result.error().message() << '\n';
        return 1;
    }
    
    // List loaded plugins
    for (const auto& plugin : registry.plugins()) {
        std::cout << "Plugin: " << plugin.name 
                  << " v" << plugin.version << '\n';
        std::cout << "Description: " << plugin.description << '\n';
        std::cout << "Commands: ";
        for (size_t i = 0; i < plugin.commands.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << plugin.commands[i];
        }
        std::cout << '\n';
    }
    
    return 0;
}
```

### Using the Discover Function

For a simple one-shot discovery without maintaining a registry:

```cpp
#include <iostream>
#include <nizaw/plugin.hpp>

int main() {
    const auto result = nizaw::plugin::discover("./plugins");
    if (!result) {
        std::cerr << "Failed to discover plugins: " << result.error().message() << '\n';
        return 1;
    }
    
    for (const auto& plugin : result.value()) {
        std::cout << plugin.name << "@" << plugin.version << '\n';
    }
    
    return 0;
}
```

### Using the CLI

The Nizaw CLI includes a plugin discovery command:

```bash
# List plugins in default directory
./build/nizaw plugins list

# List plugins in custom directory
./build/nizaw plugins list /path/to/plugins
```

### Using the Plugin Loader Example

The repository includes a plugin loader example that demonstrates advanced usage:

```bash
# List all plugins
./build/examples/plugins/plugin_loader ./plugins

# Execute a command on all plugins
./build/examples/plugins/plugin_loader ./plugins hello "World"

# Execute specific command
./build/examples/plugins/plugin_loader ./plugins status
```

## Complete Example

Here's a minimal working plugin example:

```cpp
// my_plugin.cpp
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include <nizaw/plugin.hpp>

namespace {
    struct PluginState {
        bool initialized = false;
        int load_count = 0;
    };
    
    PluginState g_state;
}

// Helper to log to stderr
void log_info(const std::string& message) {
    std::fprintf(stderr, "[my-plugin] INFO: %s\n", message.c_str());
}

// Command implementation
const char* cmd_greet(const char* args) {
    static std::string result;
    const char* name = (args && args[0] != '\0') ? args : "World";
    result = "Hello, " + std::string(name) + " from my-plugin!";
    return result.c_str();
}

const char* cmd_status(const char* /*args*/) {
    static std::string result;
    result = "Status: " + std::string(g_state.initialized ? "initialized" : "not initialized") +
             ", loaded " + std::to_string(g_state.load_count) + " times";
    return result.c_str();
}

// Plugin descriptor
extern "C" const nizaw::plugin::PluginDescriptor nizaw_plugin_descriptor = {
    "my-plugin",           // name
    "1.0.0",              // version
    "A simple example plugin", // description
    nizaw::plugin::PluginApiVersion, // api_version
    "greet,status"        // commands
};

// Lifecycle functions
extern "C" int nizaw_plugin_init() {
    if (g_state.initialized) {
        return 0;
    }
    g_state.initialized = true;
    g_state.load_count++;
    log_info("Plugin initialized");
    return 0;
}

extern "C" void nizaw_plugin_cleanup() {
    if (g_state.initialized) {
        log_info("Plugin cleaned up");
        g_state.initialized = false;
    }
}

// Command execution
extern "C" const char* nizaw_plugin_execute(const char* command, const char* args) {
    if (!command) {
        return "Error: no command provided";
    }
    
    const std::string_view cmd(command);
    
    if (cmd == "greet") {
        return cmd_greet(args);
    }
    if (cmd == "status") {
        return cmd_status(args);
    }
    
    static std::string error_result;
    error_result = "Error: unknown command: " + std::string(command);
    return error_result.c_str();
}

// Optional: version function
extern "C" const char* nizaw_plugin_version() {
    return "1.0.0";
}

// Optional: commands function
extern "C" const char* nizaw_plugin_commands() {
    return "greet,status";
}
```

## Best Practices

### Memory Management

- **Static strings**: Return `static std::string` from command functions to ensure the string remains valid
- **No dynamic allocation**: Avoid `new`/`malloc` in plugins unless you manage the lifetime carefully
- **Plugin lifetime**: Remember that strings returned from plugin functions must remain valid until the plugin is unloaded

### Error Handling

- **Init failures**: Return non-zero from `nizaw_plugin_init()` if initialization fails
- **Command errors**: Return error strings starting with "Error: " for consistency
- **Null safety**: Always check for null pointers in `execute` and other functions

```cpp
extern "C" const char* nizaw_plugin_execute(const char* command, const char* args) {
    if (!command) {
        return "Error: no command provided";
    }
    
    // Your code here
    
    return "Error: unknown command";
}
```

### Logging

- **Use stderr**: All logging should go to stderr (stdout is reserved for command output)
- **Consistent format**: Use `[plugin-name] LEVEL: message` format
- **Avoid secrets**: Never log sensitive data like credentials or private keys
- **Minimal output**: Keep logging concise; users can enable verbose mode if needed

```cpp
void log_info(const std::string& message) {
    std::fprintf(stderr, "[my-plugin] INFO: %s\n", message.c_str());
}
```

### Command Design

- **Simple signatures**: Keep command functions simple: `const char* cmd(const char* args)`
- **Return strings**: Commands should return string results or error messages
- **Document commands**: Provide clear descriptions in the plugin descriptor
- **Consistent naming**: Use lowercase with underscores for command names (e.g., `list_processes`)

### Plugin Structure

- **Single namespace**: Keep all plugin code in an anonymous namespace
- **Global state**: Use a single `PluginState` struct for all plugin state
- **Minimal exports**: Only export functions that are actually needed
- **C linkage**: Always use `extern "C"` for exported functions to prevent name mangling

### Testing

- **Test plugin loading**: Verify your plugin loads correctly with `nizaw plugins list`
- **Test all commands**: Execute each command with valid and invalid arguments
- **Test cleanup**: Ensure cleanup functions are called and resources are freed
- **Test error cases**: Verify error handling for missing commands, null arguments, etc.

## Advanced Topics

### Plugin Dependencies

Plugins should avoid dependencies on other plugins. Each plugin should be self-contained and only depend on the Nizaw plugin API.

### Thread Safety

The plugin system is currently single-threaded. If you need thread safety, manage it within your plugin using `std::mutex` or other synchronization primitives.

### Plugin Communication

Plugins cannot directly communicate with each other. If you need shared state, consider:

1. Using environment variables
2. Writing to shared files
3. Implementing a client-server architecture within your plugin

### Version Compatibility

- Always check `api_version` matches `PluginApiVersion`
- The plugin system is pre-1.0, so breaking changes may occur before version 1.0.0
- Monitor the Nizaw CHANGELOG for API changes

## Troubleshooting

### Plugin not loading

1. Check file extension: must be `.so`
2. Verify `nizaw_plugin_descriptor` is exported: `nm -D my_plugin.so | grep nizaw_plugin_descriptor`
3. Check API version matches
4. Ensure all required exports are present

### Commands not appearing

1. Verify the `commands` field in the descriptor is comma-separated
2. Check that `nizaw_plugin_commands()` returns the same list
3. Ensure commands are not empty strings

### Plugin loads but commands fail

1. Check stderr for error messages from the plugin
2. Verify `nizaw_plugin_execute` is exported
3. Test commands individually using the plugin loader example

## See Also

- [examples/plugins/hello_plugin.cpp](../examples/plugins/hello_plugin.cpp) - Complete working example
- [examples/plugins/plugin_loader.cpp](../examples/plugins/plugin_loader.cpp) - Plugin loading demonstration
- [docs/api-design.md](../api-design.md) - Plugin API specifications
- [docs/wiki/Usage.md](../wiki/Usage.md) - Using the plugins command