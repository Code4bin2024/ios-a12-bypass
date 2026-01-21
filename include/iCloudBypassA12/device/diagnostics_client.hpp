/// @file diagnostics_client.hpp
/// @brief iOS diagnostics relay client for device control.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"
#include "iCloudBypassA12/device/device_manager.hpp"

namespace a12bypass {

/// Client for iOS diagnostics relay (device control operations)
class DiagnosticsClient {
public:
    explicit DiagnosticsClient(DeviceManager& device_manager);
    ~DiagnosticsClient() = default;
    
    // Non-copyable, non-movable (contains reference)
    DiagnosticsClient(const DiagnosticsClient&) = delete;
    DiagnosticsClient& operator=(const DiagnosticsClient&) = delete;
    DiagnosticsClient(DiagnosticsClient&&) = delete;
    DiagnosticsClient& operator=(DiagnosticsClient&&) = delete;
    
    /// Connect to diagnostics relay service
    [[nodiscard]] Result<void> connect();
    
    /// Disconnect from service
    void disconnect() noexcept;
    
    /// Check if connected
    [[nodiscard]] bool is_connected() const noexcept;
    
    /// Restart the device
    [[nodiscard]] Result<void> restart();
    
    /// Shutdown the device
    [[nodiscard]] Result<void> shutdown();
    
    /// Put device to sleep
    [[nodiscard]] Result<void> sleep();
    
    /// Get battery information as XML
    [[nodiscard]] Result<std::string> get_battery_info();
    
    /// Query IORegistry entry
    [[nodiscard]] Result<std::string> get_ioreg_entry(std::string_view key);

private:
    DeviceManager& device_manager_;
    DiagnosticsHandle diag_;
    
    [[nodiscard]] Result<void> ensure_connected();
};

} // namespace a12bypass
