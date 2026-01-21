/// @file epub_builder.cpp
/// @brief EPUB builder implementation.
/// @copyright MIT License

#include "iCloudBypassA12/payload/epub_builder.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <fstream>
#include <cstring>

namespace a12bypass {

Result<ByteBuffer> EpubBuilder::build_epub(ByteSpan plist_data) {
    LOG_DEBUG("Building EPUB with ", plist_data.size(), " byte plist");
    
    // Create temp file for the zip
    auto temp_path = fs::temp_directory_path() / ("a12bypass_epub_" + std::to_string(std::rand()) + ".zip");
    
    // Open zip for writing
    int err = 0;
    zip_t* archive = zip_open(temp_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::string err_msg = zip_error_strerror(&error);
        zip_error_fini(&error);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, 
            "Failed to create zip archive: " + err_msg
        );
    }
    
    // Add mimetype file (must be first, stored uncompressed for EPUB compliance)
    zip_source_t* mimetype_src = zip_source_buffer(
        archive, 
        mimetype.data(),
        mimetype.size(), 
        0
    );
    
    if (!mimetype_src) {
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to create mimetype source"
        );
    }
    
    zip_int64_t mimetype_idx = zip_file_add(
        archive, 
        std::string(mimetype_path).c_str(),
        mimetype_src, 
        ZIP_FL_OVERWRITE
    );
    
    if (mimetype_idx < 0) {
        zip_source_free(mimetype_src);
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to add mimetype to archive"
        );
    }
    
    // Set mimetype to STORED (no compression) - required for EPUB
    zip_set_file_compression(archive, static_cast<zip_uint64_t>(mimetype_idx), ZIP_CM_STORE, 0);
    
    // Add MobileGestalt plist
    // Need to copy data since zip_source_buffer doesn't own it
    void* plist_copy = malloc(plist_data.size());
    if (!plist_copy) {
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to allocate memory for plist"
        );
    }
    std::memcpy(plist_copy, plist_data.data(), plist_data.size());
    
    zip_source_t* plist_src = zip_source_buffer(
        archive, 
        plist_copy,
        plist_data.size(), 
        1  // freep=1: libzip will free the buffer
    );
    
    if (!plist_src) {
        free(plist_copy);
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to create plist source"
        );
    }
    
    zip_int64_t plist_idx = zip_file_add(
        archive, 
        std::string(plist_internal_path).c_str(),
        plist_src, 
        ZIP_FL_OVERWRITE
    );
    
    if (plist_idx < 0) {
        zip_source_free(plist_src);
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to add plist to archive"
        );
    }
    
    // Close and finalize
    if (zip_close(archive) < 0) {
        std::string err_msg = zip_strerror(archive);
        zip_discard(archive);
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, 
            "Failed to finalize archive: " + err_msg
        );
    }
    
    // Read the file back into memory
    std::ifstream file(temp_path, std::ios::binary | std::ios::ate);
    if (!file) {
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to read generated archive"
        );
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer result(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(result.data()), size)) {
        fs::remove(temp_path);
        return Result<ByteBuffer>::failure(
            ErrorCode::EpubCreationFailed, "Failed to read generated archive"
        );
    }
    
    // Clean up temp file
    fs::remove(temp_path);
    
    LOG_DEBUG("Generated EPUB: ", result.size(), " bytes");
    return Result<ByteBuffer>::success(std::move(result));
}

Result<ByteBuffer> EpubBuilder::build_epub_from_file(const fs::path& plist_file_path) {
    // Read plist file
    std::ifstream file(plist_file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Result<ByteBuffer>::failure(
            ErrorCode::FileReadFailed,
            "Failed to open plist file: " + plist_file_path.string()
        );
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer plist_data(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(plist_data.data()), size)) {
        return Result<ByteBuffer>::failure(
            ErrorCode::FileReadFailed,
            "Failed to read plist file: " + plist_file_path.string()
        );
    }
    
    return build_epub(plist_data);
}

Result<void> EpubBuilder::write_to_file(ByteSpan data, const fs::path& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Result<void>::failure(
            ErrorCode::FileWriteFailed,
            "Failed to create file: " + path.string()
        );
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), 
               static_cast<std::streamsize>(data.size()));
    
    if (!file.good()) {
        return Result<void>::failure(
            ErrorCode::FileWriteFailed,
            "Failed to write file: " + path.string()
        );
    }
    
    LOG_DEBUG("Wrote EPUB to ", path.filename().string());
    return Result<void>::success();
}

} // namespace a12bypass
