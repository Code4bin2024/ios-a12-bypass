/// @file error.hpp
/// @brief Error handling types and error codes for iOS A12+ Bypass Toolkit
/// @copyright MIT License - Educational & Research Use Only

#pragma once

#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace a12bypass {

// ============================================================================
// Error Categories
// ============================================================================

/// Error category enumeration
enum class ErrorCategory {
    None,           ///< No error
    Device,         ///< Device communication errors
    Service,        ///< iOS service errors
    FileSystem,     ///< File I/O errors
    Network,        ///< Network-related errors
    Payload,        ///< Payload generation errors
    Validation,     ///< Input validation errors
    Timeout,        ///< Operation timeout
    Cancelled,      ///< Operation was cancelled
    Internal        ///< Internal/unexpected errors
};

/// Convert error category to string
[[nodiscard]] constexpr std::string_view to_string(ErrorCategory category) noexcept {
    switch (category) {
        case ErrorCategory::None:       return "None";
        case ErrorCategory::Device:     return "Device";
        case ErrorCategory::Service:    return "Service";
        case ErrorCategory::FileSystem: return "FileSystem";
        case ErrorCategory::Network:    return "Network";
        case ErrorCategory::Payload:    return "Payload";
        case ErrorCategory::Validation: return "Validation";
        case ErrorCategory::Timeout:    return "Timeout";
        case ErrorCategory::Cancelled:  return "Cancelled";
        case ErrorCategory::Internal:   return "Internal";
    }
    return "Unknown";
}

// ============================================================================
// Error Codes
// ============================================================================

/// Specific error codes
enum class ErrorCode {
    // Success
    Success = 0,
    
    // Device errors (100-199)
    DeviceNotFound = 100,
    DeviceNotConnected = 101,
    DeviceConnectionFailed = 102,
    DeviceAlreadyConnected = 103,
    DeviceNotSupported = 104,
    DeviceAlreadyActivated = 105,
    
    // Service errors (200-299)
    ServiceStartFailed = 200,
    ServiceConnectionFailed = 201,
    ServiceOperationFailed = 202,
    LockdownFailed = 203,
    AfcOperationFailed = 204,
    DiagnosticsOperationFailed = 205,
    SyslogOperationFailed = 206,
    
    // File system errors (300-399)
    FileNotFound = 300,
    FileReadFailed = 301,
    FileWriteFailed = 302,
    DirectoryCreateFailed = 303,
    FileRemoveFailed = 304,
    
    // Payload errors (400-499)
    PayloadGenerationFailed = 400,
    SqliteCreationFailed = 401,
    EpubCreationFailed = 402,
    PlistParsingFailed = 403,
    
    // Validation errors (500-599)
    InvalidGuid = 500,
    InvalidProductType = 501,
    InvalidConfiguration = 502,
    ResourcesNotFound = 503,
    
    // Operation errors (600-699)
    OperationTimeout = 600,
    OperationCancelled = 601,
    OperationFailed = 602,
    
    // Internal errors (900-999)
    InternalError = 900,
    NotImplemented = 901,
    UnknownError = 999
};

/// Get the category for an error code
[[nodiscard]] constexpr ErrorCategory category_for(ErrorCode code) noexcept {
    const auto value = static_cast<int>(code);
    if (value == 0) return ErrorCategory::None;
    if (value >= 100 && value < 200) return ErrorCategory::Device;
    if (value >= 200 && value < 300) return ErrorCategory::Service;
    if (value >= 300 && value < 400) return ErrorCategory::FileSystem;
    if (value >= 400 && value < 500) return ErrorCategory::Payload;
    if (value >= 500 && value < 600) return ErrorCategory::Validation;
    if (value >= 600 && value < 700) {
        if (code == ErrorCode::OperationTimeout) return ErrorCategory::Timeout;
        if (code == ErrorCode::OperationCancelled) return ErrorCategory::Cancelled;
    }
    return ErrorCategory::Internal;
}

// ============================================================================
// Error Class
// ============================================================================

/// Comprehensive error type with context and source location
class Error {
public:
    /// Construct a success (no error)
    constexpr Error() noexcept = default;
    
    /// Construct an error with code and message
    explicit Error(
        ErrorCode code,
        std::string message = {},
        std::source_location location = std::source_location::current()
    ) noexcept
        : code_(code)
        , message_(std::move(message))
        , location_(location)
    {}
    
    /// Construct with just a message (uses UnknownError code)
    explicit Error(
        std::string message,
        std::source_location location = std::source_location::current()
    ) noexcept
        : code_(ErrorCode::UnknownError)
        , message_(std::move(message))
        , location_(location)
    {}
    
    // Accessors
    [[nodiscard]] constexpr ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] constexpr ErrorCategory category() const noexcept { return category_for(code_); }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const std::source_location& location() const noexcept { return location_; }
    
    /// Check if this represents an error (not success)
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return code_ != ErrorCode::Success;
    }
    
    /// Check if this is a success (no error)
    [[nodiscard]] constexpr bool is_success() const noexcept {
        return code_ == ErrorCode::Success;
    }
    
    /// Get formatted error string
    [[nodiscard]] std::string to_string() const;
    
    /// Get full diagnostic string with source location
    [[nodiscard]] std::string to_diagnostic_string() const;
    
    // Factory methods for common errors
    static Error success() noexcept { return Error{}; }
    
    static Error device_not_found(
        std::source_location loc = std::source_location::current()
    ) {
        return Error{ErrorCode::DeviceNotFound, "No iOS device found", loc};
    }
    
    static Error timeout(
        std::string_view operation = "Operation",
        std::source_location loc = std::source_location::current()
    ) {
        return Error{ErrorCode::OperationTimeout, std::string(operation) + " timed out", loc};
    }
    
    static Error cancelled(
        std::source_location loc = std::source_location::current()
    ) {
        return Error{ErrorCode::OperationCancelled, "Operation was cancelled", loc};
    }
    
    static Error invalid_guid(
        std::string_view guid,
        std::source_location loc = std::source_location::current()
    ) {
        return Error{ErrorCode::InvalidGuid, "Invalid GUID format: " + std::string(guid), loc};
    }
    
private:
    ErrorCode code_ = ErrorCode::Success;
    std::string message_;
    std::source_location location_ = std::source_location::current();
};

// ============================================================================
// Error Helper Macros
// ============================================================================

/// Create an error at the current source location
#define BYPASS_ERROR(code, msg) \
    ::a12bypass::Error{::a12bypass::ErrorCode::code, msg, std::source_location::current()}

/// Create a simple error with just a message
#define BYPASS_ERROR_MSG(msg) \
    ::a12bypass::Error{msg, std::source_location::current()}

} // namespace a12bypass
