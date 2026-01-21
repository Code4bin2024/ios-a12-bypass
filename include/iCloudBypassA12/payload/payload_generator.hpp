/// @file payload_generator.hpp
/// @brief Payload generator for bypass operation.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/bypass/config.hpp"
#include "sqlite_builder.hpp"
#include "epub_builder.hpp"

namespace a12bypass {

/// Generates all payload files needed for the bypass
class PayloadGenerator {
public:
    /// Construct with path to resources directory
    explicit PayloadGenerator(const fs::path& resources_dir);
    ~PayloadGenerator() = default;
    
    // Non-copyable but movable
    PayloadGenerator(const PayloadGenerator&) = delete;
    PayloadGenerator& operator=(const PayloadGenerator&) = delete;
    PayloadGenerator(PayloadGenerator&&) noexcept = default;
    PayloadGenerator& operator=(PayloadGenerator&&) noexcept = default;
    
    /// Generate all payload files
    /// @param product_type Device product type (e.g., "iPhone14,5")
    /// @param guid SystemGroup GUID
    /// @param output_dir Directory to write payload files
    [[nodiscard]] Result<PayloadFiles> generate(
        std::string_view product_type,
        std::string_view guid,
        const fs::path& output_dir
    );
    
    /// Get path to device-specific plist
    [[nodiscard]] fs::path get_plist_path(std::string_view product_type) const;
    
    /// Check if device is supported
    [[nodiscard]] bool is_device_supported(std::string_view product_type) const;
    
    /// List all supported devices
    [[nodiscard]] std::vector<std::string> list_supported_devices() const;

private:
    fs::path resources_dir_;
    SqliteBuilder sqlite_builder_;
    EpubBuilder epub_builder_;
    
    /// Normalize product type (iPhone14,5 -> iPhone14-5)
    [[nodiscard]] static std::string normalize_product_type(std::string_view product_type);
    
    /// Generate iTunesMetadata.plist
    [[nodiscard]] Result<fs::path> generate_itunes_metadata(const fs::path& output_dir);
};

} // namespace a12bypass
