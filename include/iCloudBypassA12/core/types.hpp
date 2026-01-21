/// @file types.hpp
/// @brief Core type definitions, strong types, and fundamental constants for iOS A12+ Bypass Toolkit
/// @copyright MIT License - Educational & Research Use Only

#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace a12bypass {

// ============================================================================
// Namespace Aliases
// ============================================================================

namespace fs = std::filesystem;
namespace chrono = std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

// ============================================================================
// Type Aliases
// ============================================================================

/// Byte buffer type for binary data
using ByteBuffer = std::vector<std::uint8_t>;

/// View into byte data without ownership
using ByteSpan = std::span<const std::uint8_t>;

/// Mutable view into byte data
using MutableByteSpan = std::span<std::uint8_t>;

// ============================================================================
// Strong Types
// ============================================================================

/// Strong type wrapper for compile-time type safety
template<typename T, typename Tag>
class StrongType {
public:
    constexpr StrongType() = default;
    constexpr explicit StrongType(T value) : value_(std::move(value)) {}
    
    [[nodiscard]] constexpr T& get() noexcept { return value_; }
    [[nodiscard]] constexpr const T& get() const noexcept { return value_; }
    
    [[nodiscard]] constexpr explicit operator T() const noexcept { return value_; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept 
        requires requires { static_cast<bool>(std::declval<T>()); }
    {
        return !value_.empty();
    }

    constexpr auto operator<=>(const StrongType&) const = default;
    
private:
    T value_{};
};

// Strong type tags
struct UdidTag {};
struct GuidTag {};
struct ProductTypeTag {};
struct SerialNumberTag {};

/// Unique Device Identifier (40-character hex string)
using Udid = StrongType<std::string, UdidTag>;

/// System Group GUID (UUID format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)
using Guid = StrongType<std::string, GuidTag>;

/// Product type identifier (e.g., "iPhone14,5")
using ProductType = StrongType<std::string, ProductTypeTag>;

/// Device serial number
using SerialNumber = StrongType<std::string, SerialNumberTag>;

// ============================================================================
// Device Information
// ============================================================================

/// Complete device information structure
struct DeviceInfo {
    Udid udid;
    ProductType product_type;
    std::string product_version;
    SerialNumber serial_number;
    std::string device_name;
    std::string activation_state;
    std::string build_version;
    std::string hardware_model;
    std::string device_class;
    
    /// Check if the device is already activated
    [[nodiscard]] constexpr bool is_activated() const noexcept {
        return activation_state == "Activated";
    }
    
    /// Get normalized product type with dashes instead of commas
    [[nodiscard]] std::string normalized_product_type() const {
        std::string result = product_type.get();
        for (char& c : result) {
            if (c == ',') c = '-';
        }
        return result;
    }
};

// ============================================================================
// Device Path Constants
// ============================================================================

namespace paths {

/// AFC paths on the device filesystem
namespace device {
    inline constexpr std::string_view downloads_dir = "/Downloads";
    inline constexpr std::string_view downloads_db = "/Downloads/downloads.28.sqlitedb";
    inline constexpr std::string_view downloads_wal = "/Downloads/downloads.28.sqlitedb-wal";
    inline constexpr std::string_view downloads_shm = "/Downloads/downloads.28.sqlitedb-shm";
    inline constexpr std::string_view books_dir = "/Books";
    inline constexpr std::string_view books_metadata = "/Books/iTunesMetadata.plist";
    inline constexpr std::string_view books_asset = "/Books/asset.epub";
    inline constexpr std::string_view itunes_dir = "/iTunes_Control/iTunes";
    inline constexpr std::string_view itunes_metadata = "/iTunes_Control/iTunes/iTunesMetadata.plist";
} // namespace device

/// Path traversal to MobileGestalt cache
inline constexpr std::string_view gestalt_cache_traversal = 
    "../../../../../../private/var/containers/Shared/SystemGroup/"
    "systemgroup.com.apple.mobilegestaltcache/Library";

} // namespace paths

// ============================================================================
// Callback Types
// ============================================================================

/// Progress callback for long-running operations
using ProgressCallback = std::function<void(
    std::string_view stage,
    int percent,
    std::string_view message
)>;

/// Device event callback for connection/disconnection events
using DeviceEventCallback = std::function<void(
    std::string_view udid,
    bool connected
)>;

/// Syslog line callback
using SyslogCallback = std::function<void(std::string_view line)>;

// ============================================================================
// Version Information
// ============================================================================

namespace version {
    inline constexpr std::string_view string = 
#ifdef BYPASS_VERSION
        BYPASS_VERSION;
#else
        "2.1.0";
#endif

    inline constexpr int major = 2;
    inline constexpr int minor = 1;
    inline constexpr int patch = 0;
} // namespace version

} // namespace a12bypass
