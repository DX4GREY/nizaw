#include "nizaw/plugin.hpp"

extern "C" const nizaw::plugin::PluginDescriptor nizaw_plugin_descriptor = {
    "test-plugin",
    "0.1.0",
    "A simple test plugin for Nizaw phase 9.",
    nizaw::plugin::PluginApiVersion,
    "test-plugin-command"
};
