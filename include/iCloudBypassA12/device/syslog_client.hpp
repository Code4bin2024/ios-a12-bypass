/// @file syslog_client.hpp
/// @brief iOS syslog relay client for log collection.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"
#include "iCloudBypassA12/device/device_manager.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace a12bypass {

/// Client for iOS syslog relay (log collection)
class SyslogClient {
public:
    explicit SyslogClient(DeviceManager& device_manager);
    ~SyslogClient();
    
    // Not copyable or movable due to thread
    SyslogClient(const SyslogClient&) = delete;
    SyslogClient& operator=(const SyslogClient&) = delete;
    SyslogClient(SyslogClient&&) = delete;
    SyslogClient& operator=(SyslogClient&&) = delete;
    
    /// Connect to syslog relay service
    [[nodiscard]] Result<void> connect();
    
    /// Disconnect from service
    void disconnect() noexcept;
    
    /// Check if connected
    [[nodiscard]] bool is_connected() const noexcept;
    
    /// Start receiving syslogs with callback
    [[nodiscard]] Result<void> start(SyslogCallback callback);
    
    /// Stop receiving syslogs
    void stop() noexcept;
    
    /// Check if currently receiving
    [[nodiscard]] bool is_running() const noexcept;
    
    /// Collect logs and search for a pattern
    /// Returns the first matching line or error if not found
    [[nodiscard]] Result<std::string> collect_and_search(
        chrono::seconds duration,
        std::string_view search_pattern,
        std::string_view process_filter = {}
    );
    
    /// Collect all logs for a duration
    [[nodiscard]] Result<std::vector<std::string>> collect(chrono::seconds duration);
    
    /// Get collected logs
    [[nodiscard]] const std::vector<std::string>& logs() const;
    
    /// Clear collected logs
    void clear_logs();

private:
    DeviceManager& device_manager_;
    SyslogHandle syslog_;
    
    std::atomic<bool> running_{false};
    SyslogCallback callback_;
    
    std::vector<std::string> logs_;
    mutable std::mutex logs_mutex_;
    
    std::string line_buffer_;
    
    /// Static callback for libimobiledevice
    static void syslog_callback(char c, void* user_data);
};

} // namespace a12bypass
