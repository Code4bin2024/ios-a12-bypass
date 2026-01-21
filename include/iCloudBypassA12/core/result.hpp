/// @file result.hpp
/// @brief Result type for error handling (similar to std::expected from C++23).
/// @copyright MIT License

#pragma once

#include "error.hpp"
#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace a12bypass {

// Forward declaration
template<typename T>
class Result;

// ============================================================================
// Concepts
// ============================================================================

template<typename T>
concept Movable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

// ============================================================================
// Result<T> - Primary template for non-void types
// ============================================================================

/// Result type for operations that may fail.
/// Similar to std::expected<T, Error> but available in C++20.
template<typename T>
class [[nodiscard]] Result {
    static_assert(!std::is_void_v<T>, "Use Result<void> for void results");
    static_assert(Movable<T>, "T must be movable");
    
public:
    using value_type = T;
    using error_type = Error;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    /// Default construct with default value (success)
    Result() requires std::is_default_constructible_v<T>
        : data_(std::in_place_index<0>, T{})
    {}
    
    /// Construct with a value (success)
    Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : data_(std::in_place_index<0>, std::move(value))
    {}
    
    /// Construct with an error (failure)
    Result(Error error) noexcept
        : data_(std::in_place_index<1>, std::move(error))
    {}
    
    /// Construct with error code and message
    Result(ErrorCode code, std::string message)
        : data_(std::in_place_index<1>, Error{code, std::move(message)})
    {}
    
    // Copy and move
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;
    
    // ========================================================================
    // Observers
    // ========================================================================
    
    /// Check if the result contains a value
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return data_.index() == 0;
    }
    
    /// Conversion to bool - true if contains value
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }
    
    /// Get the value (undefined behavior if no value)
    [[nodiscard]] constexpr T& value() & {
        return std::get<0>(data_);
    }
    
    [[nodiscard]] constexpr const T& value() const& {
        return std::get<0>(data_);
    }
    
    [[nodiscard]] constexpr T&& value() && {
        return std::get<0>(std::move(data_));
    }
    
    [[nodiscard]] constexpr const T&& value() const&& {
        return std::get<0>(std::move(data_));
    }
    
    /// Get the error (undefined behavior if has value)
    [[nodiscard]] constexpr Error& error() & {
        return std::get<1>(data_);
    }
    
    [[nodiscard]] constexpr const Error& error() const& {
        return std::get<1>(data_);
    }
    
    [[nodiscard]] constexpr Error&& error() && {
        return std::get<1>(std::move(data_));
    }
    
    /// Get value or return default
    template<typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) const& {
        if (has_value()) {
            return value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }
    
    template<typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) && {
        if (has_value()) {
            return std::move(value());
        }
        return static_cast<T>(std::forward<U>(default_value));
    }
    
    /// Pointer-like access
    [[nodiscard]] constexpr T* operator->() noexcept {
        return &value();
    }
    
    [[nodiscard]] constexpr const T* operator->() const noexcept {
        return &value();
    }
    
    [[nodiscard]] constexpr T& operator*() & noexcept {
        return value();
    }
    
    [[nodiscard]] constexpr const T& operator*() const& noexcept {
        return value();
    }
    
    // ========================================================================
    // Monadic Operations
    // ========================================================================
    
    /// Transform the value if present
    template<typename F>
    [[nodiscard]] auto map(F&& func) const& {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) {
            return Result<U>{std::invoke(std::forward<F>(func), value())};
        }
        return Result<U>{error()};
    }
    
    template<typename F>
    [[nodiscard]] auto map(F&& func) && {
        using U = std::invoke_result_t<F, T&&>;
        if (has_value()) {
            return Result<U>{std::invoke(std::forward<F>(func), std::move(value()))};
        }
        return Result<U>{std::move(error())};
    }
    
    /// Chain another operation that returns a Result
    template<typename F>
    [[nodiscard]] auto and_then(F&& func) const& -> std::invoke_result_t<F, const T&> {
        if (has_value()) {
            return std::invoke(std::forward<F>(func), value());
        }
        using ResultType = std::invoke_result_t<F, const T&>;
        return ResultType{error()};
    }
    
    template<typename F>
    [[nodiscard]] auto and_then(F&& func) && -> std::invoke_result_t<F, T&&> {
        if (has_value()) {
            return std::invoke(std::forward<F>(func), std::move(value()));
        }
        using ResultType = std::invoke_result_t<F, T&&>;
        return ResultType{std::move(error())};
    }
    
    /// Transform the error if present
    template<typename F>
    [[nodiscard]] Result<T> map_error(F&& func) const& {
        if (has_value()) {
            return *this;
        }
        return Result<T>{std::invoke(std::forward<F>(func), error())};
    }
    
    /// Execute function if error and return alternative
    template<typename F>
    [[nodiscard]] Result<T> or_else(F&& func) const& {
        if (has_value()) {
            return *this;
        }
        return std::invoke(std::forward<F>(func), error());
    }
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    /// Create a successful result
    [[nodiscard]] static Result success(T value) {
        return Result{std::move(value)};
    }
    
    /// Create a failed result
    [[nodiscard]] static Result failure(Error error) {
        return Result{std::move(error)};
    }
    
    [[nodiscard]] static Result failure(ErrorCode code, std::string message = {}) {
        return Result{Error{code, std::move(message)}};
    }

private:
    std::variant<T, Error> data_;
};

// ============================================================================
// Result<void> - Specialization for operations without return value
// ============================================================================

/// Specialization for void results (operations that don't return a value)
template<>
class [[nodiscard]] Result<void> {
public:
    using value_type = void;
    using error_type = Error;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    /// Default construct as success
    constexpr Result() noexcept = default;
    
    /// Construct with an error (failure)
    Result(Error error) noexcept : error_(std::move(error)) {}
    
    /// Construct with error code and message
    Result(ErrorCode code, std::string message)
        : error_(Error{code, std::move(message)})
    {}
    
    // ========================================================================
    // Observers
    // ========================================================================
    
    /// Check if the result is successful
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return !error_.has_value();
    }
    
    /// Conversion to bool - true if success
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }
    
    /// Get the error (undefined behavior if successful)
    [[nodiscard]] const Error& error() const& {
        return *error_;
    }
    
    [[nodiscard]] Error&& error() && {
        return std::move(*error_);
    }
    
    // ========================================================================
    // Monadic Operations
    // ========================================================================
    
    /// Chain another operation
    template<typename F>
    [[nodiscard]] auto and_then(F&& func) const -> std::invoke_result_t<F> {
        if (has_value()) {
            return std::invoke(std::forward<F>(func));
        }
        using ResultType = std::invoke_result_t<F>;
        return ResultType{error()};
    }
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    /// Create a successful result
    [[nodiscard]] static Result success() noexcept {
        return Result{};
    }
    
    /// Create a failed result
    [[nodiscard]] static Result failure(Error error) {
        return Result{std::move(error)};
    }
    
    [[nodiscard]] static Result failure(ErrorCode code, std::string message = {}) {
        return Result{Error{code, std::move(message)}};
    }

private:
    std::optional<Error> error_;
};

// ============================================================================
// Type Aliases
// ============================================================================

/// Void result alias
using VoidResult = Result<void>;

} // namespace a12bypass
