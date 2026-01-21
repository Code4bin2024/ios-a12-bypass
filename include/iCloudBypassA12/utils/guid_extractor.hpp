/// @file guid_extractor.hpp
/// @brief GUID extraction and validation utilities.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include <optional>
#include <regex>
#include <string>
#include <string_view>

namespace a12bypass {

/// Utilities for extracting and validating SystemGroup GUIDs
class GuidExtractor {
public:
    /// Validate GUID format (UUID v4)
    [[nodiscard]] static bool validate_guid(std::string_view guid);
    
    /// Extract GUID from a syslog line
    [[nodiscard]] static std::optional<std::string> extract_from_line(
        std::string_view line
    );
    
    /// Extract GUID from syslog archive directory (macOS only)
    [[nodiscard]] static std::optional<std::string> extract_from_archive(
        const fs::path& archive_path
    );
    
    /// Extract GUID from raw syslog text
    [[nodiscard]] static std::optional<std::string> extract_from_text(
        std::string_view syslog_text
    );
    
    /// Normalize GUID to uppercase
    [[nodiscard]] static std::string normalize(std::string_view guid);

private:
    /// UUID regex pattern
    static const std::regex guid_pattern;
    
    /// Target filename in syslogs
    static constexpr std::string_view bldb_filename = "BLDatabaseManager.sqlite";
    
    /// Target path prefix
    static constexpr std::string_view target_path = 
        "/private/var/containers/Shared/SystemGroup/";
};

} // namespace a12bypass
