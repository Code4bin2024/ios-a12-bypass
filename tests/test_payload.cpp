/// @file test_payload.cpp
/// @brief Unit tests for payload components (sqlite_builder, epub_builder).
/// @copyright MIT License

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "iCloudBypassA12/payload/sqlite_builder.hpp"
#include "iCloudBypassA12/payload/epub_builder.hpp"
#include "iCloudBypassA12/bypass/config.hpp"
#include <fstream>
#include <cstdio>

using namespace a12bypass;

// ============================================================================
// Test Fixtures
// ============================================================================

namespace {
    /// RAII temp directory for tests
    class TempDir {
    public:
        TempDir() {
            path_ = fs::temp_directory_path() / ("a12bypass_test_" + std::to_string(std::rand()));
            fs::create_directories(path_);
        }
        
        ~TempDir() {
            std::error_code ec;
            fs::remove_all(path_, ec);
        }
        
        const fs::path& path() const { return path_; }
        fs::path file(const std::string& name) const { return path_ / name; }
        
    private:
        fs::path path_;
    };
    
    /// Check if data is a valid SQLite database
    bool is_valid_sqlite(ByteSpan data) {
        if (data.size() < 16) return false;
        
        // SQLite header magic: "SQLite format 3\0"
        return std::string_view(reinterpret_cast<const char*>(data.data()), 15) == "SQLite format 3";
    }
    
    /// Check if data is a valid ZIP archive
    bool is_valid_zip(ByteSpan data) {
        if (data.size() < 4) return false;
        
        // ZIP magic number: PK\x03\x04
        return data[0] == 'P' && data[1] == 'K' &&
               data[2] == 0x03 && data[3] == 0x04;
    }
}

// ============================================================================
// SqliteBuilder Tests
// ============================================================================

TEST_CASE("SqliteBuilder - Downloads database creation", "[sqlite][downloads]") {
    SqliteBuilder builder;
    
    SECTION("Build downloads database") {
        const std::string test_guid = "TESTGUID-1234-5678-9ABC-DEF012345678";
        const std::string bldb_url = "file:///path/to/BLDatabaseManager.sqlite";
        
        auto result = builder.build_downloads_db(test_guid, bldb_url);
        REQUIRE(result.has_value());
        REQUIRE(!result.value().empty());
        REQUIRE(is_valid_sqlite(result.value()));
    }
    
    SECTION("Database has minimum size") {
        auto result = builder.build_downloads_db("test-guid", "file:///test");
        REQUIRE(result.has_value());
        
        // SQLite databases have minimum overhead (at least one page)
        REQUIRE(result.value().size() >= 512);
    }
}

TEST_CASE("SqliteBuilder - BLDB database creation", "[sqlite][bldb]") {
    SqliteBuilder builder;
    
    SECTION("Build BLDB database") {
        const std::string asset_url = "../../../../../../private/var/path/to/asset";
        
        auto result = builder.build_bldb(asset_url);
        REQUIRE(result.has_value());
        REQUIRE(!result.value().empty());
        REQUIRE(is_valid_sqlite(result.value()));
    }
    
    SECTION("BLDB has reasonable size") {
        auto result = builder.build_bldb("test://url");
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() >= 512);
    }
}

TEST_CASE("SqliteBuilder - File operations", "[sqlite][file]") {
    TempDir temp;
    SqliteBuilder builder;
    
    SECTION("Write database to file") {
        auto result = builder.build_downloads_db("test-guid", "file:///test");
        REQUIRE(result.has_value());
        
        auto output = temp.file("test.sqlite");
        auto write_result = SqliteBuilder::write_to_file(result.value(), output);
        REQUIRE(write_result.has_value());
        REQUIRE(fs::exists(output));
        REQUIRE(fs::file_size(output) == result.value().size());
    }
    
    SECTION("Load database from file") {
        // First write
        auto result = builder.build_bldb("test://url");
        REQUIRE(result.has_value());
        
        auto file = temp.file("test.sqlite");
        auto write_result = SqliteBuilder::write_to_file(result.value(), file);
        REQUIRE(write_result.has_value());
        
        // Then load
        auto load_result = SqliteBuilder::load_from_file(file);
        REQUIRE(load_result.has_value());
        REQUIRE(load_result.value().size() == result.value().size());
    }
    
    SECTION("Load from non-existent file fails") {
        auto result = SqliteBuilder::load_from_file(temp.file("nonexistent.sqlite"));
        REQUIRE_FALSE(result.has_value());
    }
}

// ============================================================================
// EpubBuilder Tests
// ============================================================================

TEST_CASE("EpubBuilder - EPUB/ZIP creation", "[epub][zip]") {
    EpubBuilder builder;
    
    SECTION("Build minimal EPUB") {
        // Create some test plist data
        std::string plist_content = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>test</key>
    <string>value</string>
</dict>
</plist>)";
        ByteBuffer plist_data(plist_content.begin(), plist_content.end());
        
        auto result = builder.build_epub(plist_data);
        REQUIRE(result.has_value());
        REQUIRE(!result.value().empty());
        REQUIRE(is_valid_zip(result.value()));
    }
    
    SECTION("EPUB has non-zero size") {
        ByteBuffer small_data = {'t', 'e', 's', 't'};
        auto result = builder.build_epub(small_data);
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() > 0);
    }
}

TEST_CASE("EpubBuilder - File operations", "[epub][file]") {
    TempDir temp;
    EpubBuilder builder;
    
    SECTION("Write EPUB to file") {
        ByteBuffer test_data = {'t', 'e', 's', 't'};
        auto result = builder.build_epub(test_data);
        REQUIRE(result.has_value());
        
        auto output = temp.file("test.epub");
        auto write_result = EpubBuilder::write_to_file(result.value(), output);
        REQUIRE(write_result.has_value());
        REQUIRE(fs::exists(output));
    }
    
    SECTION("Build from plist file") {
        // Create a test plist file
        auto plist_file = temp.file("test.plist");
        {
            std::ofstream f{plist_file};
            f << R"(<?xml version="1.0"?><plist><dict/></plist>)";
        }
        
        auto result = builder.build_epub_from_file(plist_file);
        REQUIRE(result.has_value());
        REQUIRE(is_valid_zip(result.value()));
    }
    
    SECTION("Build from non-existent file fails") {
        auto result = builder.build_epub_from_file(temp.file("nonexistent.plist"));
        REQUIRE_FALSE(result.has_value());
    }
}

// ============================================================================
// PayloadFiles Tests
// ============================================================================

TEST_CASE("PayloadFiles - Existence check", "[payload][files]") {
    TempDir temp;
    
    PayloadFiles files{
        .downloads_db = temp.file("downloads.28.sqlitedb"),
        .bldb = temp.file("BLDatabaseManager.sqlite"),
        .epub = temp.file("asset.epub"),
        .itunes_metadata = temp.file("iTunesMetadata.plist")
    };
    
    SECTION("all_exist returns false when files missing") {
        REQUIRE_FALSE(files.all_exist());
    }
    
    SECTION("all_exist returns true when all files present") {
        // Create all files
        std::ofstream{files.downloads_db} << "test";
        std::ofstream{files.bldb} << "test";
        std::ofstream{files.epub} << "test";
        std::ofstream{files.itunes_metadata} << "test";
        
        REQUIRE(files.all_exist());
    }
    
    SECTION("all_exist returns false when some files missing") {
        std::ofstream{files.downloads_db} << "test";
        std::ofstream{files.bldb} << "test";
        // epub and metadata missing
        
        REQUIRE_FALSE(files.all_exist());
    }
}

TEST_CASE("PayloadFiles - Size calculation", "[payload][files]") {
    TempDir temp;
    
    PayloadFiles files{
        .downloads_db = temp.file("downloads.28.sqlitedb"),
        .bldb = temp.file("BLDatabaseManager.sqlite"),
        .epub = temp.file("asset.epub"),
        .itunes_metadata = temp.file("iTunesMetadata.plist")
    };
    
    SECTION("total_size returns 0 when no files") {
        REQUIRE(files.total_size() == 0);
    }
    
    SECTION("total_size sums all file sizes") {
        // Create files with known sizes
        {
            std::ofstream f{files.downloads_db};
            f << std::string(100, 'a');
        }
        {
            std::ofstream f{files.bldb};
            f << std::string(200, 'b');
        }
        {
            std::ofstream f{files.epub};
            f << std::string(300, 'c');
        }
        {
            std::ofstream f{files.itunes_metadata};
            f << std::string(400, 'd');
        }
        
        REQUIRE(files.total_size() == 1000);
    }
}

// ============================================================================
// SQL Schema Tests
// ============================================================================

TEST_CASE("SQL Schemas - Content validation", "[sql][schema]") {
    SECTION("Downloads schema has required elements") {
        std::string_view schema = sql::downloads_schema;
        
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("CREATE TABLE asset"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("pid INTEGER"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("download_id"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("local_path"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("PRIMARY KEY"));
    }
    
    SECTION("BLDB schema has required elements") {
        std::string_view schema = sql::bldb_schema;
        
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("CREATE TABLE ZBLDOWNLOADINFO"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("CREATE TABLE ZBLDOWNLOADPOLICYINFO"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("Z_PRIMARYKEY"));
        REQUIRE_THAT(std::string(schema), Catch::Matchers::ContainsSubstring("Z_METADATA"));
    }
}

// ============================================================================
// iTunes Metadata Tests
// ============================================================================

TEST_CASE("iTunes Metadata Template - Content validation", "[metadata][plist]") {
    std::string_view metadata = itunes_metadata_template;
    
    SECTION("Valid plist structure") {
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<?xml version="));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<!DOCTYPE plist"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<plist version=\"1.0\">"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("</plist>"));
    }
    
    SECTION("Required keys present") {
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<key>contentType</key>"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<key>kind</key>"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<key>asset-url</key>"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("<key>download-id</key>"));
    }
    
    SECTION("Asset URL contains traversal path") {
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("../../../../../../private/var/containers"));
        REQUIRE_THAT(std::string(metadata), Catch::Matchers::ContainsSubstring("mobilegestaltcache"));
    }
}

// ============================================================================
// Integration: Full Payload Generation
// ============================================================================

TEST_CASE("Payload Generation - Full workflow", "[payload][integration]") {
    TempDir temp;
    SqliteBuilder sqlite_builder;
    EpubBuilder epub_builder;
    
    const std::string test_guid = "TESTGUID-1234-5678-9ABC-DEF012345678";
    
    SECTION("Generate all payload files") {
        PayloadFiles files{
            .downloads_db = temp.file("downloads.28.sqlitedb"),
            .bldb = temp.file("BLDatabaseManager.sqlite"),
            .epub = temp.file("asset.epub"),
            .itunes_metadata = temp.file("iTunesMetadata.plist")
        };
        
        // Generate downloads DB
        auto downloads_result = sqlite_builder.build_downloads_db(test_guid, "file:///test");
        REQUIRE(downloads_result.has_value());
        auto write1 = SqliteBuilder::write_to_file(downloads_result.value(), files.downloads_db);
        REQUIRE(write1.has_value());
        
        // Generate BLDB
        auto bldb_result = sqlite_builder.build_bldb(std::string(paths::gestalt_cache_traversal));
        REQUIRE(bldb_result.has_value());
        auto write2 = SqliteBuilder::write_to_file(bldb_result.value(), files.bldb);
        REQUIRE(write2.has_value());
        
        // Generate EPUB
        ByteBuffer test_plist = {'<', 'p', 'l', 'i', 's', 't', '/', '>'};
        auto epub_result = epub_builder.build_epub(test_plist);
        REQUIRE(epub_result.has_value());
        auto write3 = EpubBuilder::write_to_file(epub_result.value(), files.epub);
        REQUIRE(write3.has_value());
        
        // Generate metadata (just copy template for test)
        {
            std::ofstream f{files.itunes_metadata};
            f << itunes_metadata_template;
        }
        
        // Verify all files exist
        REQUIRE(files.all_exist());
        
        // Verify total size is reasonable
        auto total = files.total_size();
        REQUIRE(total > 1000);  // At least 1KB for all files
        INFO("Total payload size: " << total << " bytes");
    }
}
