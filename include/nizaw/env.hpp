#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace nizaw::core::env {

/// Returns the value of environment variable `name`, or `std::nullopt` if
/// it is unset. Never throws.
[[nodiscard]] std::optional<std::string> get(std::string_view name) noexcept;

/// Returns the value of environment variable `name`, or `fallback` if unset.
[[nodiscard]] std::string get_or(std::string_view name, std::string_view fallback) noexcept;

/// True if environment variable `name` is set (even to an empty string).
[[nodiscard]] bool exists(std::string_view name) noexcept;

}  // namespace nizaw::core::env
