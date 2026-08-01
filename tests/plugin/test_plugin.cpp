#include "nizaw/plugin.hpp"

#include <filesystem>
#include <iostream>

#ifndef PLUGIN_FIXTURE_DIR
#error "PLUGIN_FIXTURE_DIR must be defined by CMake"
#endif

int run_plugin_tests() {
    const std::filesystem::path plugin_dir(PLUGIN_FIXTURE_DIR);
    nizaw::plugin::Registry registry;
    const auto load_result = registry.load_directory(plugin_dir.string());
    if (!load_result) {
        std::cerr << "Plugin registry failed: " << load_result.error().message() << std::endl;
        return 1;
    }

    if (registry.empty()) {
        std::cerr << "No plugins discovered in " << plugin_dir << std::endl;
        return 1;
    }

    const auto& plugins = registry.plugins();
    const auto& plugin = plugins.front();
    if (plugin.name != "test-plugin") {
        std::cerr << "Unexpected plugin name: " << plugin.name << std::endl;
        return 1;
    }
    if (plugin.api_version != nizaw::plugin::PluginApiVersion) {
        std::cerr << "Unexpected plugin API version: " << plugin.api_version << std::endl;
        return 1;
    }
    if (plugin.commands.empty()) {
        std::cerr << "Plugin commands not discovered" << std::endl;
        return 1;
    }
    if (plugin.commands.front() != "test-plugin-command") {
        std::cerr << "Unexpected plugin command: " << plugin.commands.front() << std::endl;
        return 1;
    }

    return 0;
}
