#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::plugin {

inline constexpr const char* PluginApiVersion = "nizaw-plugin-v1";

struct PluginDescriptor {
    const char* name;
    const char* version;
    const char* description;
    const char* api_version;
    const char* commands;
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string api_version;
    std::vector<std::string> commands;
    std::string path;
};

class Registry {
public:
    Registry() = default;
    ~Registry();
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;

    [[nodiscard]] Result<void> load_directory(std::string_view directory);
    [[nodiscard]] const std::vector<PluginInfo>& plugins() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::vector<void*> handles_;
    std::vector<PluginInfo> plugins_;
};

[[nodiscard]] Result<std::vector<PluginInfo>> discover(std::string_view directory);

}  // namespace nizaw::plugin
