#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include "nizaw/core/error.hpp"

namespace nizaw {

/// A minimal `std::expected`-like type (C++20 predates `std::expected`,
/// which only arrived in C++23). Holds either a `T` or a `nizaw::Error`.
///
/// Every public Nizaw function that can fail for a reason a caller should
/// be able to handle (missing file, permission denied, unsupported
/// telemetry, ...) returns `Result<T>` rather than throwing. Calling
/// `value()` on a `Result` that holds an `Error` is a programmer error (the
/// caller didn't check `operator bool()` first) and throws
/// `std::logic_error` — this is NOT part of the expected-failure contract,
/// it exists purely to fail loudly on misuse rather than silently returning
/// a garbage value.
template <typename T>
class Result {
public:
    // NOLINTNEXTLINE(google-explicit-constructor) — implicit by design, so
    // `return value;` / `return Error(...);` both work naturally at call sites.
    Result(T value) : storage_(std::move(value)) {}
    Result(Error error) : storage_(std::move(error)) {}

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const T& value() const& {
        if (!has_value()) {
            throw std::logic_error(
                "nizaw::Result::value() called on a Result holding an Error — "
                "check operator bool() before calling value()");
        }
        return std::get<T>(storage_);
    }
    [[nodiscard]] T& value() & {
        if (!has_value()) {
            throw std::logic_error(
                "nizaw::Result::value() called on a Result holding an Error — "
                "check operator bool() before calling value()");
        }
        return std::get<T>(storage_);
    }
    [[nodiscard]] T&& value() && {
        if (!has_value()) {
            throw std::logic_error(
                "nizaw::Result::value() called on a Result holding an Error — "
                "check operator bool() before calling value()");
        }
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] const Error& error() const& {
        if (has_value()) {
            throw std::logic_error(
                "nizaw::Result::error() called on a Result holding a value");
        }
        return std::get<Error>(storage_);
    }

    /// Returns the held value, or `fallback` if this Result holds an Error.
    template <typename U>
    [[nodiscard]] T value_or(U&& fallback) const& {
        return has_value() ? std::get<T>(storage_) : static_cast<T>(std::forward<U>(fallback));
    }

    const T& operator*() const& { return value(); }
    T& operator*() & { return value(); }
    const T* operator->() const { return &value(); }
    T* operator->() { return &value(); }

private:
    std::variant<T, Error> storage_;
};

/// Specialization for fallible operations with no meaningful success value
/// (e.g. "did this succeed or not"). Modeled separately from `Result<T>`
/// rather than via `Result<std::monostate>` so callers write the natural
/// `Result<void>` in signatures.
template <>
class Result<void> {
public:
    Result() : error_(std::nullopt) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(Error error) : error_(std::move(error)) {}

    [[nodiscard]] bool has_value() const noexcept { return !error_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const Error& error() const& {
        if (has_value()) {
            throw std::logic_error(
                "nizaw::Result<void>::error() called on a Result holding no error");
        }
        return *error_;
    }

private:
    std::optional<Error> error_;
};

}  // namespace nizaw
