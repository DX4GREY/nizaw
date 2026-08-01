#include "nizaw/core/env.hpp"

#include <cstdlib>
#include <string>

namespace nizaw::core::env {

std::optional<std::string> get(std::string_view name) noexcept {
    std::string key{name};
    const char* value = std::getenv(key.c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

std::string get_or(std::string_view name, std::string_view fallback) noexcept {
    const auto value = get(name);
    return value ? *value : std::string(fallback);
}

bool exists(std::string_view name) noexcept {
    return get(name).has_value();
}

}  // namespace nizaw::core::env
