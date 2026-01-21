/// @file plist_utils.hpp
/// @brief Property list parsing utilities.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/raii.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace a12bypass {

/// Utilities for parsing and manipulating property lists
class PlistUtils {
public:
    /// Parse plist from file
    [[nodiscard]] static std::optional<std::unordered_map<std::string, std::string>>
    parse_file(const fs::path& path);
    
    /// Parse plist from memory
    [[nodiscard]] static std::optional<std::unordered_map<std::string, std::string>>
    parse_data(ByteSpan data);
    
    /// Parse plist from XML string
    [[nodiscard]] static std::optional<std::unordered_map<std::string, std::string>>
    parse_xml(std::string_view xml);
    
    /// Read raw plist file content
    [[nodiscard]] static std::optional<ByteBuffer> read_file(const fs::path& path);
    
    /// Write plist data to file
    [[nodiscard]] static bool write_file(const fs::path& path, ByteSpan data);
    
    /// Convert XML plist to binary plist
    [[nodiscard]] static std::optional<ByteBuffer> xml_to_binary(std::string_view xml);
    
    /// Convert binary plist to XML
    [[nodiscard]] static std::optional<std::string> binary_to_xml(ByteSpan data);

private:
    /// Helper to extract string values from plist node
    static void extract_dict_values(
        plist_t node,
        std::unordered_map<std::string, std::string>& result
    );
};

} // namespace a12bypass
