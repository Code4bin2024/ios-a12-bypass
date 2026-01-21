/// @file guid_extractor.cpp
/// @brief GUID extractor implementation.
/// @copyright MIT License

#include "iCloudBypassA12/utils/guid_extractor.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <sstream>

namespace a12bypass {

const std::regex GuidExtractor::guid_pattern{
    "[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}"
};

bool GuidExtractor::validate_guid(std::string_view guid) {
    std::string guid_str{guid};
    if (!std::regex_match(guid_str, guid_pattern)) {
        return false;
    }
    
    // Check UUID v4 format
    // Version should be '4' (position 14)
    // Variant should be '8', '9', 'A', or 'B' (position 19)
    std::string upper = normalize(guid);
    
    char version = upper[14];
    char variant = upper[19];
    
    bool valid_version = (version == '4');
    bool valid_variant = (variant == '8' || variant == '9' || 
                          variant == 'A' || variant == 'B');
    
    return valid_version && valid_variant;
}

std::optional<std::string> GuidExtractor::extract_from_line(std::string_view line) {
    // Check if line contains BLDatabaseManager.sqlite
    if (line.find(bldb_filename) == std::string_view::npos) {
        return std::nullopt;
    }
    
    // Search for GUID pattern
    std::string line_str{line};
    std::smatch match;
    if (std::regex_search(line_str, match, guid_pattern)) {
        std::string guid = normalize(match[0].str());
        LOG_DEBUG("Found GUID candidate: ", guid);
        return guid;
    }
    
    return std::nullopt;
}

std::optional<std::string> GuidExtractor::extract_from_archive(const fs::path& archive_path) {
    LOG_INFO("Searching for GUID in log archive...");
    
#ifdef BYPASS_PLATFORM_MACOS
    // Use macOS 'log' command to parse the logarchive
    std::ostringstream cmd;
    cmd << "/usr/bin/log show "
        << "--archive \"" << archive_path.string() << "\" "
        << "--info --debug "
        << "--style syslog "
        << "--predicate 'process == \"bookassetd\" AND eventMessage CONTAINS \"" 
        << bldb_filename << "\"'";
    
    // Execute command and capture output
    std::array<char, 4096> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(cmd.str().c_str(), "r"), pclose
    );
    
    if (!pipe) {
        LOG_ERROR("Failed to execute log command");
        return std::nullopt;
    }
    
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return extract_from_text(result);
#else
    (void)archive_path;
    LOG_ERROR("Log archive parsing is only supported on macOS");
    return std::nullopt;
#endif
}

std::optional<std::string> GuidExtractor::extract_from_text(std::string_view syslog_text) {
    std::istringstream stream{std::string{syslog_text}};
    std::string line;
    
    while (std::getline(stream, line)) {
        // Check for bookassetd and BLDatabaseManager
        if (line.find("bookassetd") != std::string::npos && 
            line.find(bldb_filename) != std::string::npos) {
            
            LOG_DEBUG("Found relevant log line: ", 
                      line.substr(0, std::min<std::size_t>(100, line.size())));
            
            auto guid = extract_from_line(line);
            if (guid) {
                LOG_SUCCESS("Extracted GUID: ", *guid);
                return guid;
            }
        }
    }
    
    LOG_WARN("GUID not found in syslog data");
    return std::nullopt;
}

std::string GuidExtractor::normalize(std::string_view guid) {
    std::string result{guid};
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

} // namespace a12bypass
