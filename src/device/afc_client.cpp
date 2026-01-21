/// @file afc_client.cpp
/// @brief AFC client implementation.
/// @copyright MIT License

#include "iCloudBypassA12/device/afc_client.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include "iCloudBypassA12/bypass/config.hpp"
#include <fstream>
#include <cstring>

namespace a12bypass {

AfcClient::AfcClient(DeviceManager& device_manager)
    : device_manager_(device_manager)
{}

Result<void> AfcClient::connect() {
    if (!device_manager_.is_connected()) {
        return Result<void>::failure(
            ErrorCode::DeviceNotConnected, "Device not connected"
        );
    }
    
    disconnect();
    
    ServiceDescriptorHandle service;
    lockdownd_error_t lerr = lockdownd_start_service(
        device_manager_.lockdown_handle(),
        "com.apple.afc",
        service.addressof()
    );
    
    if (lerr != LOCKDOWN_E_SUCCESS || !service) {
        return Result<void>::failure(
            ErrorCode::ServiceStartFailed, "Failed to start AFC service"
        );
    }
    
    afc_client_t raw_afc = nullptr;
    afc_error_t err = afc_client_new(
        device_manager_.device_handle(),
        service.get(),
        &raw_afc
    );
    
    if (err != AFC_E_SUCCESS || !raw_afc) {
        return Result<void>::failure(
            ErrorCode::ServiceConnectionFailed, "Failed to create AFC client"
        );
    }
    
    afc_.reset(raw_afc);
    LOG_DEBUG("AFC client connected");
    return Result<void>::success();
}

void AfcClient::disconnect() noexcept {
    afc_.reset();
}

bool AfcClient::is_connected() const noexcept {
    return static_cast<bool>(afc_);
}

Result<void> AfcClient::ensure_connected() {
    if (!is_connected()) {
        return connect();
    }
    return Result<void>::success();
}

Result<void> AfcClient::push_file(
    const fs::path& local_path,
    std::string_view remote_path
) {
    auto conn_result = ensure_connected();
    if (!conn_result) return conn_result;
    
    // Read local file
    std::ifstream file(local_path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Result<void>::failure(
            ErrorCode::FileReadFailed,
            "Failed to open local file: " + local_path.string()
        );
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer data(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return Result<void>::failure(
            ErrorCode::FileReadFailed, "Failed to read local file"
        );
    }
    file.close();
    
    LOG_DEBUG("Pushing ", local_path.filename().string(), 
              " (", size, " bytes) to ", remote_path);
    
    // Remove existing file if present (ignore result)
    (void)remove_file(remote_path);
    
    // Open remote file for writing
    std::uint64_t handle = 0;
    const std::string remote_str{remote_path};
    afc_error_t err = afc_file_open(
        afc_.get(), 
        remote_str.c_str(),
        AFC_FOPEN_WRONLY, 
        &handle
    );
    
    if (err != AFC_E_SUCCESS) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to open remote file for writing"
        );
    }
    
    // Write data
    std::uint32_t written = 0;
    err = afc_file_write(
        afc_.get(), 
        handle,
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::uint32_t>(data.size()),
        &written
    );
    afc_file_close(afc_.get(), handle);
    
    if (err != AFC_E_SUCCESS || written != data.size()) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to write to remote file"
        );
    }
    
    LOG_SUCCESS("Pushed ", local_path.filename().string(), " to device");
    return Result<void>::success();
}

Result<void> AfcClient::pull_file(
    std::string_view remote_path,
    const fs::path& local_path
) {
    auto data_result = read_file(remote_path);
    if (!data_result) {
        return Result<void>::failure(data_result.error());
    }
    
    std::ofstream file(local_path, std::ios::binary);
    if (!file) {
        return Result<void>::failure(
            ErrorCode::FileWriteFailed,
            "Failed to create local file: " + local_path.string()
        );
    }
    
    const auto& data = data_result.value();
    file.write(reinterpret_cast<const char*>(data.data()), 
               static_cast<std::streamsize>(data.size()));
    
    if (!file.good()) {
        return Result<void>::failure(
            ErrorCode::FileWriteFailed, "Failed to write local file"
        );
    }
    
    LOG_SUCCESS("Pulled ", remote_path, " to ", local_path.filename().string());
    return Result<void>::success();
}

Result<ByteBuffer> AfcClient::read_file(std::string_view remote_path) {
    auto conn_result = ensure_connected();
    if (!conn_result) {
        return Result<ByteBuffer>::failure(conn_result.error());
    }
    
    // Get file size first
    auto size_result = file_size(remote_path);
    if (!size_result) {
        return Result<ByteBuffer>::failure(size_result.error());
    }
    
    const std::string remote_str{remote_path};
    std::uint64_t handle = 0;
    afc_error_t err = afc_file_open(
        afc_.get(),
        remote_str.c_str(),
        AFC_FOPEN_RDONLY,
        &handle
    );
    
    if (err != AFC_E_SUCCESS) {
        return Result<ByteBuffer>::failure(
            ErrorCode::AfcOperationFailed, "Failed to open remote file"
        );
    }
    
    ByteBuffer data(static_cast<std::size_t>(size_result.value()));
    std::uint32_t bytes_read = 0;
    
    err = afc_file_read(
        afc_.get(),
        handle,
        reinterpret_cast<char*>(data.data()),
        static_cast<std::uint32_t>(data.size()),
        &bytes_read
    );
    afc_file_close(afc_.get(), handle);
    
    if (err != AFC_E_SUCCESS) {
        return Result<ByteBuffer>::failure(
            ErrorCode::AfcOperationFailed, "Failed to read remote file"
        );
    }
    
    data.resize(bytes_read);
    return Result<ByteBuffer>::success(std::move(data));
}

Result<void> AfcClient::write_file(
    std::string_view remote_path,
    ByteSpan data
) {
    auto conn_result = ensure_connected();
    if (!conn_result) return conn_result;
    
    (void)remove_file(remote_path);
    
    const std::string remote_str{remote_path};
    std::uint64_t handle = 0;
    afc_error_t err = afc_file_open(
        afc_.get(),
        remote_str.c_str(),
        AFC_FOPEN_WRONLY,
        &handle
    );
    
    if (err != AFC_E_SUCCESS) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to open remote file for writing"
        );
    }
    
    std::uint32_t written = 0;
    err = afc_file_write(
        afc_.get(),
        handle,
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::uint32_t>(data.size()),
        &written
    );
    afc_file_close(afc_.get(), handle);
    
    if (err != AFC_E_SUCCESS || written != data.size()) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to write remote file"
        );
    }
    
    return Result<void>::success();
}

Result<void> AfcClient::remove_file(std::string_view remote_path) {
    auto conn_result = ensure_connected();
    if (!conn_result) return conn_result;
    
    const std::string remote_str{remote_path};
    afc_error_t err = afc_remove_path(afc_.get(), remote_str.c_str());
    
    // ENOENT is OK - file doesn't exist
    if (err != AFC_E_SUCCESS && err != AFC_E_OBJECT_NOT_FOUND) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to remove remote file"
        );
    }
    
    return Result<void>::success();
}

Result<std::vector<std::string>> AfcClient::list_directory(std::string_view path) {
    auto conn_result = ensure_connected();
    if (!conn_result) {
        return Result<std::vector<std::string>>::failure(conn_result.error());
    }
    
    char** entries = nullptr;
    const std::string path_str{path};
    afc_error_t err = afc_read_directory(afc_.get(), path_str.c_str(), &entries);
    
    if (err != AFC_E_SUCCESS || !entries) {
        return Result<std::vector<std::string>>::failure(
            ErrorCode::AfcOperationFailed, "Failed to read directory"
        );
    }
    
    std::vector<std::string> result;
    for (char** p = entries; *p; ++p) {
        if (std::strcmp(*p, ".") != 0 && std::strcmp(*p, "..") != 0) {
            result.emplace_back(*p);
        }
        free(*p);
    }
    free(entries);
    
    return Result<std::vector<std::string>>::success(std::move(result));
}

Result<void> AfcClient::make_directory(std::string_view path) {
    auto conn_result = ensure_connected();
    if (!conn_result) return conn_result;
    
    const std::string path_str{path};
    afc_error_t err = afc_make_directory(afc_.get(), path_str.c_str());
    
    if (err != AFC_E_SUCCESS && err != AFC_E_OBJECT_EXISTS) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed, "Failed to create directory"
        );
    }
    
    return Result<void>::success();
}

Result<void> AfcClient::remove_directory(std::string_view path, bool recursive) {
    auto conn_result = ensure_connected();
    if (!conn_result) return conn_result;
    
    if (recursive) {
        auto entries = list_directory(path);
        if (entries) {
            for (const auto& entry : entries.value()) {
                std::string full_path = std::string(path) + "/" + entry;
                auto info = get_file_info(full_path);
                if (info && info.value()["st_ifmt"] == "S_IFDIR") {
                    (void)remove_directory(full_path, true);
                } else {
                    (void)remove_file(full_path);
                }
            }
        }
    }
    
    return remove_file(path);
}

Result<bool> AfcClient::file_exists(std::string_view path) {
    auto info = get_file_info(path);
    return Result<bool>::success(info.has_value());
}

Result<std::uint64_t> AfcClient::file_size(std::string_view path) {
    auto info = get_file_info(path);
    if (!info) {
        return Result<std::uint64_t>::failure(info.error());
    }
    
    auto it = info.value().find("st_size");
    if (it == info.value().end()) {
        return Result<std::uint64_t>::failure(
            ErrorCode::AfcOperationFailed, "Size not found in file info"
        );
    }
    
    return Result<std::uint64_t>::success(std::stoull(it->second));
}

Result<std::unordered_map<std::string, std::string>> 
AfcClient::get_file_info(std::string_view path) {
    auto conn_result = ensure_connected();
    if (!conn_result) {
        return Result<std::unordered_map<std::string, std::string>>::failure(
            conn_result.error()
        );
    }
    
    char** info = nullptr;
    const std::string path_str{path};
    afc_error_t err = afc_get_file_info(afc_.get(), path_str.c_str(), &info);
    
    if (err != AFC_E_SUCCESS || !info) {
        return Result<std::unordered_map<std::string, std::string>>::failure(
            ErrorCode::FileNotFound, "File not found"
        );
    }
    
    std::unordered_map<std::string, std::string> result;
    for (char** p = info; *p; p += 2) {
        result[p[0]] = p[1];
        free(p[0]);
        free(p[1]);
    }
    free(info);
    
    return Result<std::unordered_map<std::string, std::string>>::success(
        std::move(result)
    );
}

Result<void> AfcClient::clean_bypass_files() {
    LOG_INFO("Cleaning old bypass files from device...");
    
    // Remove all known bypass-related files (ignore individual results)
    (void)remove_file(paths::device::downloads_db);
    (void)remove_file(paths::device::downloads_wal);
    (void)remove_file(paths::device::downloads_shm);
    (void)remove_file(paths::device::books_asset);
    (void)remove_file(paths::device::books_metadata);
    (void)remove_file(paths::device::itunes_metadata);
    (void)remove_file(std::string(paths::device::itunes_metadata) + ".ext");
    
    LOG_SUCCESS("Cleaned bypass files");
    return Result<void>::success();
}

} // namespace a12bypass
