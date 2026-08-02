// plugin_loader.cpp
//
// Example program that loads, displays, and executes Nizaw plugins.
//
// This program demonstrates:
//   1. Loading plugins using nizaw::plugin::Registry
//   2. Displaying plugin metadata
//   3. Calling functions exported by plugins via dlsym()
//   4. Executing commands provided by plugins
//
// Usage:
//   ./plugin_loader <plugin-directory> [command] [args...]
//
// Examples:
//   ./plugin_loader ./build/examples/plugins
//   ./plugin_loader ./build/examples/plugins hello
//   ./plugin_loader ./build/examples/plugins greet "Budi"
//   ./plugin_loader ./build/examples/plugins status
//   ./plugin_loader ./build/examples/plugins echo "Hello world"

#include <dlfcn.h>
#include <iostream>
#include <string>
#include <vector>

#include <nizaw/plugin.hpp>

namespace {

// Function types exported by plugins.
using InitFn = int (*)();
using CleanupFn = void (*)();
using ExecuteFn = const char* (*)(const char*, const char*);
using VersionFn = const char* (*)();
using CommandsFn = const char* (*)();

// Helper to get functions from shared library.
template <typename Fn>
Fn get_symbol(void* handle, const char* name) {
    dlerror();
    auto symbol = reinterpret_cast<Fn>(dlsym(handle, name));
    const char* error = dlerror();
    if (error != nullptr) {
        return nullptr;
    }
    return symbol;
}

// Display plugin metadata.
void print_plugin_info(const nizaw::plugin::PluginInfo& plugin) {
    std::cout << "  Name         : " << plugin.name << '\n';
    std::cout << "  Version      : " << plugin.version << '\n';
    std::cout << "  Description  : " << plugin.description << '\n';
    std::cout << "  API Version  : " << plugin.api_version << '\n';
    std::cout << "  Path         : " << plugin.path << '\n';

    std::cout << "  Commands     : ";
    for (size_t i = 0; i < plugin.commands.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << plugin.commands[i];
    }
    std::cout << "\n";
}

// Execute command on plugin.
void execute_command(void* handle, const std::string& command, const std::string& args) {
    auto execute = get_symbol<ExecuteFn>(handle, "nizaw_plugin_execute");
    if (!execute) {
        std::cerr << "  [ERROR] Plugin does not export nizaw_plugin_execute()\n";
        return;
    }

    const char* result = execute(command.c_str(), args.empty() ? nullptr : args.c_str());
    std::cout << "  Result: " << (result ? result : "(null)") << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin-directory> [command] [args...]\n";
        std::cerr << "\nAvailable commands (if plugin supports):\n";
        std::cerr << "  hello [name]  - Greet the user\n";
        std::cerr << "  greet [name]  - Greet with format\n";
        std::cerr << "  info          - Display plugin info\n";
        std::cerr << "  status        - Display internal plugin status\n";
        std::cerr << "  reset         - Reset plugin state\n";
        std::cerr << "  echo [text]   - Return text\n";
        return 1;
    }

    const std::string plugin_dir = argv[1];
    const std::string command = (argc >= 3) ? argv[2] : "";
    std::string args;
    for (int i = 3; i < argc; ++i) {
        if (!args.empty()) args += " ";
        args += argv[i];
    }

    // Create plugin registry and load all plugins from directory.
    nizaw::plugin::Registry registry;
    const auto result = registry.load_directory(plugin_dir);
    if (!result) {
        std::cerr << "Failed to load plugins from '" << plugin_dir
                  << "': " << result.error().message() << '\n';
        return 1;
    }

    const auto& plugins = registry.plugins();
    if (plugins.empty()) {
        std::cout << "No plugins found in '" << plugin_dir << "'\n";
        return 0;
    }

    std::cout << "Found " << plugins.size() << " plugin(s) in '"
              << plugin_dir << "':\n\n";

    for (const auto& plugin : plugins) {
        print_plugin_info(plugin);
        std::cout << "\n";

        // Open plugin handle manually to access exported functions.
        // Registry stores internal handles, but for demonstration
        // we open them ourselves.
        void* handle = dlopen(plugin.path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::cerr << "  [ERROR] Failed to open plugin: " << dlerror() << "\n\n";
            continue;
        }

        // Call init function if available.
        auto init = get_symbol<InitFn>(handle, "nizaw_plugin_init");
        if (init) {
            std::cout << "  [INIT] ";
            const int init_result = init();
            std::cout << (init_result == 0 ? "success" : "failed (code " + std::to_string(init_result) + ")") << "\n";
        }

        // Display dynamic version if available.
        auto version = get_symbol<VersionFn>(handle, "nizaw_plugin_version");
        if (version) {
            std::cout << "  [VERSION] " << version() << "\n";
        }

        // Display dynamic command list if available.
        auto commands = get_symbol<CommandsFn>(handle, "nizaw_plugin_commands");
        if (commands) {
            std::cout << "  [COMMANDS] " << commands() << "\n";
        }

        // Execute command if requested.
        if (!command.empty()) {
            std::cout << "\n  [EXECUTE] Command: '" << command << "'"
                      << (args.empty() ? "" : " with arguments: '" + args + "'") << "\n";
            execute_command(handle, command, args);
        }

        // Call cleanup function if available.
        auto cleanup = get_symbol<CleanupFn>(handle, "nizaw_plugin_cleanup");
        if (cleanup) {
            std::cout << "\n  [CLEANUP] ";
            cleanup();
            std::cout << "done\n";
        }

        dlclose(handle);
        std::cout << "\n";
    }

    return 0;
}