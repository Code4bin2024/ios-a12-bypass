/// @file payload_generator.cpp
/// @brief Payload generator implementation.
/// @copyright MIT License

#include "iCloudBypassA12/payload/payload_generator.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <fstream>
#include <algorithm>

namespace a12bypass {

PayloadGenerator::PayloadGenerator(const fs::path& resources_dir)
    : resources_dir_(resources_dir)
{}

Result<PayloadFiles> PayloadGenerator::generate(
    std::string_view product_type,
    std::string_view guid,
    const fs::path& output_dir
) {
    LOG_STEP("Generating Payloads");
    LOG_INFO("Device: ", product_type, ", GUID: ", guid);
    
    // Ensure output directory exists
    std::error_code ec;
    fs::create_directories(output_dir, ec);
    if (ec) {
        return Result<PayloadFiles>::failure(
            ErrorCode::DirectoryCreateFailed,
            "Failed to create output directory: " + ec.message()
        );
    }
    
    PayloadFiles files;
    files.downloads_db = output_dir / "downloads.28.sqlitedb";
    files.bldb = output_dir / "BLDatabaseManager.sqlite";
    files.epub = output_dir / "fixedfile";
    files.itunes_metadata = output_dir / "iTunesMetadata.plist";
    
    // Step 1: Get device-specific plist
    auto plist_path = get_plist_path(product_type);
    if (!fs::exists(plist_path)) {
        return Result<PayloadFiles>::failure(
            ErrorCode::DeviceNotSupported,
            "Device not supported: " + std::string(product_type) + 
            " (plist not found at " + plist_path.string() + ")"
        );
    }
    LOG_INFO("Using plist: ", plist_path.filename().string());
    
    // Step 2: Build EPUB with MobileGestalt plist
    auto epub_result = epub_builder_.build_epub_from_file(plist_path);
    if (!epub_result) {
        return Result<PayloadFiles>::failure(epub_result.error());
    }
    
    auto write_result = EpubBuilder::write_to_file(epub_result.value(), files.epub);
    if (!write_result) {
        return Result<PayloadFiles>::failure(write_result.error());
    }
    LOG_SUCCESS("Created EPUB payload");
    
    // Step 3: Build BLDatabaseManager.sqlite
    std::string asset_url = "file://" + files.epub.string();
    
    auto bldb_result = sqlite_builder_.build_bldb(asset_url);
    if (!bldb_result) {
        return Result<PayloadFiles>::failure(bldb_result.error());
    }
    
    write_result = SqliteBuilder::write_to_file(bldb_result.value(), files.bldb);
    if (!write_result) {
        return Result<PayloadFiles>::failure(write_result.error());
    }
    LOG_SUCCESS("Created BLDatabaseManager.sqlite");
    
    // Step 4: Build downloads.28.sqlitedb
    std::string bldb_url = "file://" + files.bldb.string();
    
    auto downloads_result = sqlite_builder_.build_downloads_db(guid, bldb_url);
    if (!downloads_result) {
        return Result<PayloadFiles>::failure(downloads_result.error());
    }
    
    write_result = SqliteBuilder::write_to_file(downloads_result.value(), files.downloads_db);
    if (!write_result) {
        return Result<PayloadFiles>::failure(write_result.error());
    }
    LOG_SUCCESS("Created downloads.28.sqlitedb");
    
    // Step 5: Generate iTunesMetadata.plist
    auto metadata_result = generate_itunes_metadata(output_dir);
    if (!metadata_result) {
        return Result<PayloadFiles>::failure(metadata_result.error());
    }
    files.itunes_metadata = metadata_result.value();
    LOG_SUCCESS("Created iTunesMetadata.plist");
    
    LOG_SUCCESS("All payloads generated successfully");
    return Result<PayloadFiles>::success(std::move(files));
}

fs::path PayloadGenerator::get_plist_path(std::string_view product_type) const {
    std::string normalized = normalize_product_type(product_type);
    return resources_dir_ / "plists" / normalized / "com.apple.MobileGestalt.plist";
}

bool PayloadGenerator::is_device_supported(std::string_view product_type) const {
    return fs::exists(get_plist_path(product_type));
}

std::vector<std::string> PayloadGenerator::list_supported_devices() const {
    std::vector<std::string> devices;
    
    auto plists_dir = resources_dir_ / "plists";
    if (!fs::exists(plists_dir)) {
        return devices;
    }
    
    for (const auto& entry : fs::directory_iterator(plists_dir)) {
        if (entry.is_directory()) {
            auto plist_file = entry.path() / "com.apple.MobileGestalt.plist";
            if (fs::exists(plist_file)) {
                // Convert iPhone14-5 back to iPhone14,5
                std::string name = entry.path().filename().string();
                std::replace(name.begin(), name.end(), '-', ',');
                devices.push_back(name);
            }
        }
    }
    
    std::sort(devices.begin(), devices.end());
    return devices;
}

std::string PayloadGenerator::normalize_product_type(std::string_view product_type) {
    std::string result{product_type};
    std::replace(result.begin(), result.end(), ',', '-');
    return result;
}

Result<fs::path> PayloadGenerator::generate_itunes_metadata(const fs::path& output_dir) {
    auto path = output_dir / "iTunesMetadata.plist";
    
    std::ofstream file(path);
    if (!file) {
        return Result<fs::path>::failure(
            ErrorCode::FileWriteFailed,
            "Failed to create iTunesMetadata.plist"
        );
    }
    
    file << itunes_metadata_template;
    
    if (!file.good()) {
        return Result<fs::path>::failure(
            ErrorCode::FileWriteFailed,
            "Failed to write iTunesMetadata.plist"
        );
    }
    
    return Result<fs::path>::success(path);
}

} // namespace a12bypass
