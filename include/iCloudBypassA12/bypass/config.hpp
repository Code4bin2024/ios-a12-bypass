/// @file config.hpp
/// @brief Activation bypass configuration, stage definitions, and operational constants
/// @copyright MIT License - Educational & Research Use Only

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include <chrono>
#include <optional>

namespace a12bypass {

// ============================================================================
// Bypass Stages
// ============================================================================

/// Enumeration of bypass procedure stages
enum class BypassStage : std::uint8_t {
    Init,                ///< Initial state
    DetectDevice,        ///< Detecting connected device
    ExtractGuid,         ///< Extracting SystemGroup GUID
    GeneratePayloads,    ///< Generating bypass payloads
    CleanDevice,         ///< Cleaning old bypass files
    UploadPayload,       ///< Uploading payloads to device
    Stage1Reboot,        ///< Stage 1: First reboot
    Stage1Transfer,      ///< Stage 1: Transfer metadata
    Stage2Reboot,        ///< Stage 2: Second reboot
    Stage2Transfer,      ///< Stage 2: Transfer back
    FinalReboot,         ///< Final reboot
    Complete,            ///< Bypass complete
    Failed               ///< Bypass failed
};

/// Convert stage to user-friendly string
[[nodiscard]] constexpr std::string_view to_string(BypassStage stage) noexcept {
    switch (stage) {
        case BypassStage::Init:             return "Initializing";
        case BypassStage::DetectDevice:     return "Detecting Device";
        case BypassStage::ExtractGuid:      return "Extracting GUID";
        case BypassStage::GeneratePayloads: return "Generating Payloads";
        case BypassStage::CleanDevice:      return "Cleaning Device";
        case BypassStage::UploadPayload:    return "Uploading Payload";
        case BypassStage::Stage1Reboot:     return "Stage 1 Reboot";
        case BypassStage::Stage1Transfer:   return "Stage 1 Transfer";
        case BypassStage::Stage2Reboot:     return "Stage 2 Reboot";
        case BypassStage::Stage2Transfer:   return "Stage 2 Transfer";
        case BypassStage::FinalReboot:      return "Final Reboot";
        case BypassStage::Complete:         return "Complete";
        case BypassStage::Failed:           return "Failed";
    }
    return "Unknown";
}

/// Get progress percentage for a stage
[[nodiscard]] constexpr int progress_for(BypassStage stage) noexcept {
    switch (stage) {
        case BypassStage::Init:             return 0;
        case BypassStage::DetectDevice:     return 5;
        case BypassStage::ExtractGuid:      return 15;
        case BypassStage::GeneratePayloads: return 30;
        case BypassStage::CleanDevice:      return 40;
        case BypassStage::UploadPayload:    return 50;
        case BypassStage::Stage1Reboot:     return 60;
        case BypassStage::Stage1Transfer:   return 65;
        case BypassStage::Stage2Reboot:     return 75;
        case BypassStage::Stage2Transfer:   return 80;
        case BypassStage::FinalReboot:      return 95;
        case BypassStage::Complete:         return 100;
        case BypassStage::Failed:           return 0;
    }
    return 0;
}

// ============================================================================
// Bypass Configuration
// ============================================================================

/// Configuration for the bypass procedure
struct BypassConfig {
    /// Pre-configured GUID (skips extraction if set)
    std::optional<Guid> preset_guid;
    
    /// Enable automatic mode (no prompts)
    bool auto_mode = false;
    
    /// Enable verbose/debug logging
    bool verbose = false;
    
    /// Maximum GUID extraction attempts
    int max_guid_attempts = 5;
    
    /// Timeout for device reconnection after reboot
    chrono::seconds reboot_timeout{300};
    
    /// Timeout for syslog collection
    chrono::seconds syslog_timeout{200};
    
    /// Path to resources directory
    fs::path resources_path;
    
    /// Validate the configuration
    [[nodiscard]] bool validate() const noexcept {
        if (preset_guid && preset_guid->get().empty()) {
            return false;
        }
        if (max_guid_attempts < 1) {
            return false;
        }
        if (reboot_timeout.count() < 30) {
            return false;
        }
        return true;
    }
    
    /// Get resources path, with fallback logic
    [[nodiscard]] fs::path effective_resources_path() const {
        if (!resources_path.empty() && fs::exists(resources_path)) {
            return resources_path;
        }
        
        // Try common locations
        static const fs::path candidates[] = {
            fs::current_path() / "resources",
            #ifdef BYPASS_RESOURCES_DIR
            fs::path(BYPASS_RESOURCES_DIR),
            #endif
            "/usr/local/share/a12_bypass/resources",
            "/opt/homebrew/share/a12_bypass/resources"
        };
        
        for (const auto& path : candidates) {
            if (fs::exists(path / "plists")) {
                return path;
            }
        }
        
        return {};
    }
};

// ============================================================================
// Stage Information
// ============================================================================

/// Information about the current bypass stage
struct StageInfo {
    BypassStage stage;
    std::string_view name;
    std::string_view description;
    int progress_percent;
    
    /// Create stage info from a stage enum
    [[nodiscard]] static constexpr StageInfo from(BypassStage stage) noexcept {
        return StageInfo{
            .stage = stage,
            .name = to_string(stage),
            .description = to_string(stage),
            .progress_percent = progress_for(stage)
        };
    }
};

// ============================================================================
// Payload Files
// ============================================================================

/// Generated payload file paths
struct PayloadFiles {
    fs::path downloads_db;       ///< downloads.28.sqlitedb
    fs::path bldb;               ///< BLDatabaseManager.sqlite
    fs::path epub;               ///< asset.epub (fixedfile)
    fs::path itunes_metadata;    ///< iTunesMetadata.plist
    
    /// Check if all required files exist
    [[nodiscard]] bool all_exist() const {
        return fs::exists(downloads_db) &&
               fs::exists(bldb) &&
               fs::exists(epub) &&
               fs::exists(itunes_metadata);
    }
    
    /// Get total size of all files
    [[nodiscard]] std::uintmax_t total_size() const {
        std::uintmax_t total = 0;
        if (fs::exists(downloads_db)) total += fs::file_size(downloads_db);
        if (fs::exists(bldb)) total += fs::file_size(bldb);
        if (fs::exists(epub)) total += fs::file_size(epub);
        if (fs::exists(itunes_metadata)) total += fs::file_size(itunes_metadata);
        return total;
    }
};

// ============================================================================
// SQL Templates
// ============================================================================

namespace sql {

/// Schema for downloads.28.sqlitedb
inline constexpr std::string_view downloads_schema = R"SQL(
PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE asset (
    pid INTEGER,
    download_id INTEGER,
    asset_order INTEGER DEFAULT 0,
    asset_type TEXT,
    bytes_total INTEGER,
    url TEXT,
    local_path TEXT,
    destination_url TEXT,
    path_extension TEXT,
    retry_count INTEGER,
    http_method TEXT,
    initial_odr_size INTEGER,
    is_discretionary INTEGER DEFAULT 0,
    is_downloaded INTEGER DEFAULT 0,
    is_drm_free INTEGER DEFAULT 0,
    is_external INTEGER DEFAULT 0,
    is_hls INTEGER DEFAULT 0,
    is_local_cache_server INTEGER DEFAULT 0,
    is_zip_streamable INTEGER DEFAULT 0,
    processing_types INTEGER DEFAULT 0,
    video_dimensions TEXT,
    timeout_interval REAL,
    store_flavor TEXT,
    download_token INTEGER DEFAULT 0,
    blocked_reason INTEGER DEFAULT 0,
    avfoundation_blocked INTEGER DEFAULT 0,
    service_type INTEGER DEFAULT 0,
    protection_type INTEGER DEFAULT 0,
    store_download_key TEXT,
    etag TEXT,
    bytes_to_hash INTEGER,
    hash_type INTEGER DEFAULT 0,
    server_guid TEXT,
    file_protection TEXT,
    variant_id TEXT,
    hash_array BLOB,
    http_headers BLOB,
    request_parameters BLOB,
    body_data BLOB,
    body_data_file_path TEXT,
    sinfs_data BLOB,
    dpinfo_data BLOB,
    uncompressed_size INTEGER DEFAULT 0,
    url_session_task_id INTEGER DEFAULT -1,
    PRIMARY KEY (pid)
);
)SQL";

/// Schema for BLDatabaseManager.sqlite
inline constexpr std::string_view bldb_schema = R"SQL(
PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE ZBLDOWNLOADINFO (
    Z_PK INTEGER PRIMARY KEY,
    Z_ENT INTEGER,
    Z_OPT INTEGER,
    ZDOWNLOADID INTEGER,
    ZPRIORITY INTEGER,
    ZSTATE INTEGER,
    ZDOWNLOADPERCENT FLOAT,
    ZDOWNLOADPROGRESS FLOAT,
    ZASSETPATH VARCHAR,
    ZDOWNLOADERROR VARCHAR,
    ZDOWNLOADIDENTIFIER VARCHAR,
    ZDOWNLOADURL VARCHAR,
    ZURL VARCHAR
);
CREATE TABLE ZBLDOWNLOADPOLICYINFO (
    Z_PK INTEGER PRIMARY KEY,
    Z_ENT INTEGER,
    Z_OPT INTEGER,
    ZSTOREPLAYLISTIDENTIFIER INTEGER,
    ZPOLICYID VARCHAR,
    ZPOLICYDATA BLOB
);
CREATE TABLE Z_PRIMARYKEY (Z_ENT INTEGER PRIMARY KEY, Z_NAME VARCHAR, Z_SUPER INTEGER, Z_MAX INTEGER);
INSERT INTO Z_PRIMARYKEY VALUES(1,'BLDownloadInfo',0,6);
INSERT INTO Z_PRIMARYKEY VALUES(2,'BLDownloadPolicyInfo',0,2);
CREATE TABLE Z_METADATA (Z_VERSION INTEGER PRIMARY KEY, Z_UUID VARCHAR(255), Z_PLIST BLOB);
CREATE INDEX Z_BLDownloadInfo_byDownloadIDIndex ON ZBLDOWNLOADINFO (ZDOWNLOADID COLLATE BINARY ASC);
CREATE INDEX Z_BLDownloadInfo_byStateIndex ON ZBLDOWNLOADINFO (ZSTATE COLLATE BINARY ASC);
)SQL";

} // namespace sql

// ============================================================================
// iTunes Metadata Template
// ============================================================================

inline constexpr std::string_view itunes_metadata_template = R"PLIST(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>appleId</key>
    <string></string>
    <key>artistName</key>
    <string>Sebastian Saenz</string>
    <key>bookGenre</key>
    <string>Contemporary Romance</string>
    <key>bookId</key>
    <integer>765107106</integer>
    <key>bookVersion</key>
    <integer>1</integer>
    <key>contentType</key>
    <string>ebook</string>
    <key>drmVersionNumber</key>
    <integer>0</integer>
    <key>isCompilation</key>
    <false/>
    <key>itemName</key>
    <string>Cartas de Amor a la Luna</string>
    <key>kind</key>
    <string>book</string>
    <key>playlistId</key>
    <integer>0</integer>
    <key>playlistName</key>
    <string></string>
    <key>purchaseDate</key>
    <date>2025-05-03T15:15:53Z</date>
    <key>rating</key>
    <dict>
        <key>explicit</key>
        <string>cleaned</string>
    </dict>
    <key>releaseDate</key>
    <date>2025-05-01T00:00:00Z</date>
    <key>shortDescription</key>
    <string></string>
    <key>softwareIcon57x57URL</key>
    <string>https://is1-ssl.mzstatic.com/image/thumb/Publication126/v4/3d/b6/0a/3db60a65-b1a5-51c3-b306-c58870663fd3/Portada.jpg/57x57bb.jpg</string>
    <key>softwareIconNeedsShine</key>
    <false/>
    <key>softwareVersionExternalIdentifier</key>
    <integer>0</integer>
    <key>userRating</key>
    <dict>
        <key>rating</key>
        <real>0.0</real>
    </dict>
    <key>download-id</key>
    <string>J19N_PUB_190099164604738</string>
    <key>external-version-id</key>
    <string>190099164604738</string>
    <key>external-id</key>
    <string>765107106</string>
    <key>asset-url</key>
    <string>../../../../../../private/var/containers/Shared/SystemGroup/systemgroup.com.apple.mobilegestaltcache/Library</string>
</dict>
</plist>)PLIST";

} // namespace a12bypass
