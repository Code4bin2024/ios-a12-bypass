/// @file device_manager.hpp
/// @brief iOS device detection and connection management.
/// @copyright MIT License

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/core/raii.hpp"
#include <chrono>
#include <vector>

namespace a12bypass {

/// Manages connection to iOS devices via libimobiledevice
class DeviceManager {
public:
    // ========================================================================
    // Constructors
    // ========================================================================
    
    DeviceManager() = default;
    ~DeviceManager() = default;
    
    // Move-only
    DeviceManager(DeviceManager&&) noexcept = default;
    DeviceManager& operator=(DeviceManager&&) noexcept = default;
    
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    
    // ========================================================================
    // Device Discovery
    // ========================================================================
    
    /// List all connected device UDIDs
    [[nodiscard]] Result<std::vector<std::string>> list_devices() const;
    
    /// Detect and connect to a device
    /// @param udid Optional specific device UDID (empty = first available)
    [[nodiscard]] Result<DeviceInfo> detect_device(std::string_view udid = {});
    
    // ========================================================================
    // Connection Management
    // ========================================================================
    
    /// Connect to a device
    /// @param udid Optional specific device UDID
    [[nodiscard]] Result<void> connect(std::string_view udid = {});
    
    /// Disconnect from current device
    void disconnect() noexcept;
    
    /// Check if connected to a device
    [[nodiscard]] bool is_connected() const noexcept;
    
    /// Wait for a device to connect
    /// @param timeout Maximum time to wait
    [[nodiscard]] Result<void> wait_for_device(
        chrono::seconds timeout = chrono::seconds{180}
    );
    
    // ========================================================================
    // Device Information
    // ========================================================================
    
    /// Get current device info (must be connected)
    [[nodiscard]] const DeviceInfo& device_info() const noexcept { 
        return device_info_; 
    }
    
    // ========================================================================
    // Handle Access (for other clients)
    // ========================================================================
    
    /// Get raw device handle
    [[nodiscard]] idevice_t device_handle() const noexcept { 
        return device_.get(); 
    }
    
    /// Get raw lockdown handle
    [[nodiscard]] lockdownd_client_t lockdown_handle() const noexcept { 
        return lockdown_.get(); 
    }
    
    // ========================================================================
    // Event Handling
    // ========================================================================
    
    /// Set callback for device events
    void set_event_callback(DeviceEventCallback callback);

private:
    DeviceHandle device_;
    LockdownHandle lockdown_;
    DeviceInfo device_info_;
    DeviceEventCallback event_callback_;
    
    /// Start lockdown service
    [[nodiscard]] Result<void> start_lockdown();
    
    /// Read a string value from lockdown
    [[nodiscard]] std::optional<std::string> get_lockdown_value(
        const char* domain,
        const char* key
    ) const;
    
    /// Populate device info from lockdown
    void populate_device_info();
};

} // namespace a12bypass
