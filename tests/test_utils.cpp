/// @file test_utils.cpp
/// @brief Unit tests for utility components (guid_extractor).
/// @copyright MIT License

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "iCloudBypassA12/utils/guid_extractor.hpp"

using namespace a12bypass;

// ============================================================================
// GUID Validation Tests
// ============================================================================

// Note: validate_guid() checks for UUID v4 format specifically:
// - Position 14 must be '4' (version)
// - Position 19 must be '8', '9', 'A', or 'B' (variant)

TEST_CASE("GuidExtractor - GUID validation", "[guid][validation]") {
    SECTION("Valid UUID v4 GUIDs") {
        // Valid UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
        // where y is 8, 9, A, or B
        REQUIRE(GuidExtractor::validate_guid("12345678-1234-4234-8234-123456789ABC"));
        REQUIRE(GuidExtractor::validate_guid("12345678-1234-4234-9234-123456789ABC"));
        REQUIRE(GuidExtractor::validate_guid("12345678-1234-4234-A234-123456789ABC"));
        REQUIRE(GuidExtractor::validate_guid("12345678-1234-4234-B234-123456789ABC"));
        
        // Real-world example (UUID v4 format)
        REQUIRE(GuidExtractor::validate_guid("2A22A82B-C342-444D-972F-5270FB5080DF"));
    }
    
    SECTION("Valid GUIDs - lowercase") {
        REQUIRE(GuidExtractor::validate_guid("12345678-1234-4234-8234-123456789abc"));
        REQUIRE(GuidExtractor::validate_guid("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"));
    }
    
    SECTION("Valid GUIDs - mixed case") {
        REQUIRE(GuidExtractor::validate_guid("AaAaAaAa-BbBb-4cCc-8DdD-EeEeEeEeEeEe"));
    }
    
    SECTION("Invalid GUIDs - wrong UUID v4 version") {
        // Version not '4' at position 14
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-1234-8234-123456789ABC"));  // Version 1
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-3234-8234-123456789ABC"));  // Version 3
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-5234-8234-123456789ABC"));  // Version 5
    }
    
    SECTION("Invalid GUIDs - wrong variant") {
        // Variant not '8', '9', 'A', or 'B' at position 19
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-1234-123456789ABC"));
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-C234-123456789ABC"));
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-F234-123456789ABC"));
    }
    
    SECTION("Invalid GUIDs - wrong format") {
        // Too short
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-8234"));
        
        // Too long
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-8234-123456789ABC-FFFF"));
        
        // Wrong separators
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678_1234_4234_8234_123456789ABC"));
        REQUIRE_FALSE(GuidExtractor::validate_guid("1234567812344234823412345678ABC"));
        
        // Invalid characters
        REQUIRE_FALSE(GuidExtractor::validate_guid("GGGGGGGG-HHHH-4III-8JJJ-KKKKKKKKKKKK"));
        
        // Empty
        REQUIRE_FALSE(GuidExtractor::validate_guid(""));
        
        // Whitespace
        REQUIRE_FALSE(GuidExtractor::validate_guid(" 12345678-1234-4234-8234-123456789ABC"));
        REQUIRE_FALSE(GuidExtractor::validate_guid("12345678-1234-4234-8234-123456789ABC "));
    }
}

// ============================================================================
// GUID Normalization Tests
// ============================================================================

TEST_CASE("GuidExtractor - GUID normalization", "[guid][normalize]") {
    SECTION("Lowercase to uppercase") {
        auto normalized = GuidExtractor::normalize("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee");
        REQUIRE(normalized == "AAAAAAAA-BBBB-4CCC-8DDD-EEEEEEEEEEEE");
    }
    
    SECTION("Mixed case to uppercase") {
        auto normalized = GuidExtractor::normalize("AaAaAaAa-BbBb-4cCc-8DdD-EeEeEeEeEeEe");
        REQUIRE(normalized == "AAAAAAAA-BBBB-4CCC-8DDD-EEEEEEEEEEEE");
    }
    
    SECTION("Already uppercase unchanged") {
        auto normalized = GuidExtractor::normalize("12345678-ABCD-4FAB-8234-567890ABCDEF");
        REQUIRE(normalized == "12345678-ABCD-4FAB-8234-567890ABCDEF");
    }
}

// ============================================================================
// GUID Extraction from Syslog Line Tests
// ============================================================================

// Note: extract_from_line() requires "BLDatabaseManager.sqlite" in the line

TEST_CASE("GuidExtractor - Extract from syslog line", "[guid][extract]") {
    SECTION("Standard syslog format with GUID and BLDatabaseManager.sqlite") {
        const std::string line = 
            "Jan  4 12:34:56 iPhone kernel[0]: "
            "/private/var/containers/Shared/SystemGroup/2A22A82B-C342-444D-972F-5270FB5080DF/"
            "Library/BLDatabaseManager.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2A22A82B-C342-444D-972F-5270FB5080DF");
    }
    
    SECTION("AppleFSCompression style path with BLDatabaseManager.sqlite") {
        const std::string line = 
            "default  12:34:56.789  kernel  "
            "AppleFSCompression: failed to open /private/var/containers/Shared/SystemGroup/"
            "12345678-1234-4234-8234-123456789ABC/Library/BLDatabaseManager.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "12345678-1234-4234-8234-123456789ABC");
    }
    
    SECTION("Line without BLDatabaseManager.sqlite returns nullopt") {
        const std::string line = 
            "/private/var/containers/Shared/SystemGroup/"
            "12345678-1234-4234-8234-123456789ABC/Library/SomeOtherFile.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Line without GUID returns nullopt") {
        const std::string line = "Jan  4 12:34:56 iPhone kernel[0]: Normal log message";
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Lowercase GUID is normalized to uppercase") {
        const std::string line = 
            "/private/var/containers/Shared/SystemGroup/aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee/"
            "Library/BLDatabaseManager.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "AAAAAAAA-BBBB-4CCC-8DDD-EEEEEEEEEEEE");
    }
    
    SECTION("Empty line") {
        auto result = GuidExtractor::extract_from_line("");
        REQUIRE_FALSE(result.has_value());
    }
}

// ============================================================================
// GUID Extraction from Text Block Tests
// ============================================================================

// Note: extract_from_text() requires lines to contain both "bookassetd" AND "BLDatabaseManager.sqlite"

TEST_CASE("GuidExtractor - Extract from text block", "[guid][extract][text]") {
    SECTION("Multi-line syslog with bookassetd and BLDatabaseManager.sqlite") {
        const std::string text = R"(
Jan  4 12:34:50 iPhone kernel[0]: Normal startup message
Jan  4 12:34:51 iPhone bookassetd[123]: Starting up
Jan  4 12:34:52 iPhone bookassetd[123]: Failed to open /private/var/containers/Shared/SystemGroup/12345678-1234-4234-8234-123456789ABC/Library/BLDatabaseManager.sqlite
Jan  4 12:34:53 iPhone kernel[0]: Continuing after GUID
)";
        
        auto result = GuidExtractor::extract_from_text(text);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "12345678-1234-4234-8234-123456789ABC");
    }
    
    SECTION("No bookassetd in text returns nullopt") {
        const std::string text = R"(
Jan  4 12:34:50 iPhone kernel[0]: Normal startup message
Jan  4 12:34:51 iPhone kernel[0]: /path/to/BLDatabaseManager.sqlite
Jan  4 12:34:52 iPhone kernel[0]: No bookassetd here
)";
        
        auto result = GuidExtractor::extract_from_text(text);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("No BLDatabaseManager.sqlite in text returns nullopt") {
        const std::string text = R"(
Jan  4 12:34:50 iPhone kernel[0]: Normal startup message
Jan  4 12:34:51 iPhone bookassetd[123]: Processing asset
Jan  4 12:34:52 iPhone kernel[0]: No database file mentioned
)";
        
        auto result = GuidExtractor::extract_from_text(text);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Empty text") {
        auto result = GuidExtractor::extract_from_text("");
        REQUIRE_FALSE(result.has_value());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("GuidExtractor - Edge cases", "[guid][edge]") {
    SECTION("GUID with BLDatabaseManager.sqlite at various positions") {
        const std::string line = 
            "BLDatabaseManager.sqlite: /private/var/containers/Shared/SystemGroup/"
            "12345678-1234-4234-8234-123456789ABC/Library/BLDatabaseManager.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "12345678-1234-4234-8234-123456789ABC");
    }
    
    SECTION("Very long line with GUID and BLDatabaseManager.sqlite") {
        std::string long_prefix(5000, 'x');
        std::string line = long_prefix + 
            "/private/var/containers/Shared/SystemGroup/12345678-1234-4234-8234-123456789ABC/"
            "Library/BLDatabaseManager.sqlite";
        
        auto result = GuidExtractor::extract_from_line(line);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "12345678-1234-4234-8234-123456789ABC");
    }
}
