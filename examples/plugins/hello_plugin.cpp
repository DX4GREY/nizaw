// hello_plugin.cpp
//
// A complete and feature-rich Nizaw plugin example.
//
// This plugin demonstrates:
//   1. Plugin metadata via `nizaw_plugin_descriptor`
//   2. Exported functions for lifecycle management
//   3. Command implementations that can be invoked by the host
//   4. Logging usage
//   5. Internal plugin state
//
// To build this plugin as a shared library (.so), use CMake
// (see examples/CMakeLists.txt) or compile manually:
//
//   g++ -std=c++20 -fPIC -shared -I../../include
//       -o libhello_plugin.so hello_plugin.cpp
//
// Once built, the plugin can be loaded by the Nizaw Registry and
// the exported functions can be invoked via dlsym().

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nizaw/plugin.hpp>

// ============================================================================
// Internal constants and types
// ============================================================================

namespace {

// Internal plugin state. Stored as a global variable because the plugin
// is a shared library that is loaded once.
struct PluginState {
    bool initialized = false;
    int load_count = 0;
    std::string last_command;
    std::vector<std::string> history;
};

PluginState g_state;

// Helper to log to stderr. The plugin does not depend on the host library
// (nizaw::core) so it can be loaded independently.
void log_info(const std::string& message) {
    std::fprintf(stderr, "[hello-plugin] INFO: %s\n", message.c_str());
}

void log_warn(const std::string& message) {
    std::fprintf(stderr, "[hello-plugin] WARN: %s\n", message.c_str());
}

void log_error(const std::string& message) {
    std::fprintf(stderr, "[hello-plugin] ERROR: %s\n", message.c_str());
}

// Implementation of the "hello" command — greets the user.
const char* cmd_hello(const char* args) {
    static std::string result;
    if (args && args[0] != '\0') {
        result = "Hello, " + std::string(args) + "! Welcome to the Nizaw plugin.";
    } else {
        result = "Hello, world! Welcome to the Nizaw plugin.";
    }
    log_info("Command 'hello' executed");
    return result.c_str();
}

// Implementation of the "greet" command — greets with a formatted message.
const char* cmd_greet(const char* args) {
    static std::string result;
    const char* name = (args && args[0] != '\0') ? args : "friend";
    result = "Warm greetings to " + std::string(name) + " from hello-plugin v1.0.0!";
    log_info("Command 'greet' executed for '" + std::string(name) + "'");
    return result.c_str();
}

// Implementation of the "info" command — displays plugin and state info.
const char* cmd_info(const char* /*args*/) {
    static std::string result;
    result = "Plugin: hello-plugin\n"
             "Version: 1.0.0\n"
             "Description: A feature-rich Nizaw plugin example\n"
             "API: nizaw-plugin-v1\n"
             "Commands: hello, greet, info, status, reset, echo\n"
             "State: " + std::string(g_state.initialized ? "initialized" : "not initialized") +
             ", loaded " + std::to_string(g_state.load_count) + "x\n"
             "History: " + std::to_string(g_state.history.size()) + " commands";
    log_info("Command 'info' executed");
    return result.c_str();
}

// Implementation of the "status" command — displays internal plugin status.
const char* cmd_status(const char* /*args*/) {
    static std::string result;
    result = "Plugin status:\n"
             "  Initialized : " + std::string(g_state.initialized ? "yes" : "no") + "\n"
             "  Load count  : " + std::to_string(g_state.load_count) + "\n"
             "  Last command: " + (g_state.last_command.empty() ? "(none)" : g_state.last_command) + "\n"
             "  History     : " + std::to_string(g_state.history.size()) + " entries";
    if (!g_state.history.empty()) {
        result += "\n  Recent history:";
        const size_t start = g_state.history.size() > 5 ? g_state.history.size() - 5 : 0;
        for (size_t i = start; i < g_state.history.size(); ++i) {
            result += "\n    - " + g_state.history[i];
        }
    }
    log_info("Command 'status' executed");
    return result.c_str();
}

// Implementation of the "reset" command — resets internal plugin state.
const char* cmd_reset(const char* /*args*/) {
    g_state.history.clear();
    g_state.last_command.clear();
    g_state.load_count = 0;
    g_state.initialized = false;
    log_warn("Plugin state has been reset");
    return "Plugin state has been reset.";
}

// Implementation of the "echo" command — returns the given arguments.
const char* cmd_echo(const char* args) {
    static std::string result;
    result = args ? args : "";
    log_info("Command 'echo' executed");
    return result.c_str();
}

}  // namespace

// ============================================================================
// Symbols exported by the plugin
// ============================================================================

// Plugin metadata — the API contract between Nizaw and the plugin.
extern "C" const nizaw::plugin::PluginDescriptor nizaw_plugin_descriptor = {
    "hello-plugin",          // name
    "1.0.0",                 // version
    "A feature-rich Nizaw plugin example with command implementations.",  // description
    nizaw::plugin::PluginApiVersion,  // api_version
    "hello,greet,info,status,reset,echo"  // commands
};

// Lifecycle function: called when the plugin is first loaded.
// Returns 0 on success, non-zero on failure.
extern "C" int nizaw_plugin_init() {
    if (g_state.initialized) {
        log_warn("Plugin already initialized, skipping init");
        return 0;
    }
    g_state.initialized = true;
    g_state.load_count++;
    log_info("Plugin initialized (load #" + std::to_string(g_state.load_count) + ")");
    return 0;
}

// Lifecycle function: called when the plugin is about to be unloaded.
extern "C" void nizaw_plugin_cleanup() {
    if (g_state.initialized) {
        log_info("Plugin cleaned up");
        g_state.initialized = false;
    }
}

// Main function to execute plugin commands.
// `command` is the command name, `args` is the argument (can be null).
// Returns the result string (must remain valid while the plugin is loaded).
extern "C" const char* nizaw_plugin_execute(const char* command, const char* args) {
    if (!command) {
        return "Error: no command provided";
    }

    const std::string_view cmd(command);
    g_state.last_command = command;
    g_state.history.push_back(command);

    if (cmd == "hello") {
        return cmd_hello(args);
    }
    if (cmd == "greet") {
        return cmd_greet(args);
    }
    if (cmd == "info") {
        return cmd_info(args);
    }
    if (cmd == "status") {
        return cmd_status(args);
    }
    if (cmd == "reset") {
        return cmd_reset(args);
    }
    if (cmd == "echo") {
        return cmd_echo(args);
    }

    static std::string error_result;
    error_result = "Error: unknown command: " + std::string(command);
    log_error("Unknown command: '" + std::string(command) + "'");
    return error_result.c_str();
}

// Function to get the plugin version dynamically.
extern "C" const char* nizaw_plugin_version() {
    return "1.0.0";
}

// Function to get the list of supported commands.
extern "C" const char* nizaw_plugin_commands() {
    return "hello,greet,info,status,reset,echo";
}