/// @file epub_builder.hpp
/// @brief EPUB (ZIP) builder for bypass payloads.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"

namespace a12bypass {

/// Builder for EPUB files (actually ZIP archives) containing MobileGestalt plist
class EpubBuilder {
public:
    EpubBuilder() = default;
    ~EpubBuilder() = default;
    
    // Non-copyable but movable
    EpubBuilder(const EpubBuilder&) = delete;
    EpubBuilder& operator=(const EpubBuilder&) = delete;
    EpubBuilder(EpubBuilder&&) noexcept = default;
    EpubBuilder& operator=(EpubBuilder&&) noexcept = default;
    
    /// Build EPUB containing the MobileGestalt plist
    /// @param plist_data Device-specific MobileGestalt plist content
    [[nodiscard]] Result<ByteBuffer> build_epub(ByteSpan plist_data);
    
    /// Build EPUB from plist file path
    [[nodiscard]] Result<ByteBuffer> build_epub_from_file(const fs::path& plist_path);
    
    /// Write EPUB data to file
    [[nodiscard]] static Result<void> write_to_file(
        ByteSpan data,
        const fs::path& path
    );

private:
    // EPUB constants
    static constexpr std::string_view mimetype = "application/epub+zip";
    static constexpr std::string_view mimetype_path = "Caches/mimetype";
    static constexpr std::string_view plist_internal_path = "Caches/com.apple.MobileGestalt.plist";
};

} // namespace a12bypass
