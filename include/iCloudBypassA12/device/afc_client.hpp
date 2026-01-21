/// @file afc_client.hpp
/// @brief Apple File Conduit client for device file operations.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"
#include "iCloudBypassA12/device/device_manager.hpp"
#include <unordered_map>

namespace a12bypass {

/// Client for Apple File Conduit protocol (file operations on device)
class AfcClient {
public:
    /// Construct with reference to device manager
    explicit AfcClient(DeviceManager& device_manager);
    ~AfcClient() = default;
    
    // Non-copyable, non-movable (contains reference)
    AfcClient(const AfcClient&) = delete;
    AfcClient& operator=(const AfcClient&) = delete;
    AfcClient(AfcClient&&) = delete;
    AfcClient& operator=(AfcClient&&) = delete;
    
    // ========================================================================
    // Connection
    // ========================================================================
    
    /// Connect to AFC service
    [[nodiscard]] Result<void> connect();
    
    /// Disconnect from AFC service
    void disconnect() noexcept;
    
    /// Check if connected
    [[nodiscard]] bool is_connected() const noexcept;
    
    // ========================================================================
    // File Operations
    // ========================================================================
    
    /// Push a local file to the device
    [[nodiscard]] Result<void> push_file(
        const fs::path& local_path,
        std::string_view remote_path
    );
    
    /// Pull a file from the device to local filesystem
    [[nodiscard]] Result<void> pull_file(
        std::string_view remote_path,
        const fs::path& local_path
    );
    
    /// Read a file from device into memory
    [[nodiscard]] Result<ByteBuffer> read_file(std::string_view remote_path);
    
    /// Write data to a file on device
    [[nodiscard]] Result<void> write_file(
        std::string_view remote_path,
        ByteSpan data
    );
    
    /// Remove a file from device
    [[nodiscard]] Result<void> remove_file(std::string_view remote_path);
    
    // ========================================================================
    // Directory Operations
    // ========================================================================
    
    /// List directory contents
    [[nodiscard]] Result<std::vector<std::string>> list_directory(
        std::string_view path
    );
    
    /// Create a directory
    [[nodiscard]] Result<void> make_directory(std::string_view path);
    
    /// Remove a directory
    [[nodiscard]] Result<void> remove_directory(
        std::string_view path,
        bool recursive = false
    );
    
    // ========================================================================
    // File Information
    // ========================================================================
    
    /// Check if a file exists
    [[nodiscard]] Result<bool> file_exists(std::string_view path);
    
    /// Get file size
    [[nodiscard]] Result<std::uint64_t> file_size(std::string_view path);
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    /// Clean all bypass-related files from device
    [[nodiscard]] Result<void> clean_bypass_files();

private:
    DeviceManager& device_manager_;
    AfcHandle afc_;
    
    /// Ensure connection, connecting if necessary
    [[nodiscard]] Result<void> ensure_connected();
    
    /// Get file info as key-value pairs
    [[nodiscard]] Result<std::unordered_map<std::string, std::string>> 
    get_file_info(std::string_view path);
};

} // namespace a12bypass
