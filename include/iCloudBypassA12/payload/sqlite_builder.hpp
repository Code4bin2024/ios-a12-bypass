/// @file sqlite_builder.hpp
/// @brief SQLite database builder for bypass payloads.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"

namespace a12bypass {

/// Builder for SQLite databases used in bypass payloads
class SqliteBuilder {
public:
    SqliteBuilder() = default;
    ~SqliteBuilder() = default;
    
    // Non-copyable but movable
    SqliteBuilder(const SqliteBuilder&) = delete;
    SqliteBuilder& operator=(const SqliteBuilder&) = delete;
    SqliteBuilder(SqliteBuilder&&) noexcept = default;
    SqliteBuilder& operator=(SqliteBuilder&&) noexcept = default;
    
    /// Build downloads.28.sqlitedb for triggering itunesstored
    /// @param guid SystemGroup GUID for BLDatabaseManager path
    /// @param bldb_url URL pointing to BLDatabaseManager.sqlite
    [[nodiscard]] Result<ByteBuffer> build_downloads_db(
        std::string_view guid,
        std::string_view bldb_url
    );
    
    /// Build BLDatabaseManager.sqlite for bookassetd
    /// @param asset_url URL pointing to the EPUB asset
    [[nodiscard]] Result<ByteBuffer> build_bldb(std::string_view asset_url);
    
    /// Write database bytes to file
    [[nodiscard]] static Result<void> write_to_file(
        ByteSpan data,
        const fs::path& path
    );
    
    /// Load database from file
    [[nodiscard]] static Result<ByteBuffer> load_from_file(const fs::path& path);

private:
    /// Execute SQL statements and return serialized database
    [[nodiscard]] Result<ByteBuffer> execute_sql(std::string_view sql);
    
    /// Serialize in-memory database to bytes
    [[nodiscard]] ByteBuffer serialize_database(sqlite3* db);
};

} // namespace a12bypass
