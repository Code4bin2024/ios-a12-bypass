/// @file diagnostics_client.cpp
/// @brief Diagnostics client implementation.
/// @copyright MIT License

#include "iCloudBypassA12/device/diagnostics_client.hpp"
#include "iCloudBypassA12/core/logger.hpp"

namespace a12bypass {

DiagnosticsClient::DiagnosticsClient(DeviceManager& device_manager)
    : device_manager_(device_manager)
{}

Result<void> DiagnosticsClient::connect() {
    if (!device_manager_.is_connected()) {
        return Result<void>::failure(
            ErrorCode::DeviceNotConnected, "Device not connected"
        );
    }
    
    disconnect();
    
    ServiceDescriptorHandle service;
    lockdownd_error_t lerr = lockdownd_start_service(
        device_manager_.lockdown_handle(),
        "com.apple.mobile.diagnostics_relay",
        service.addressof()
    );
    
    if (lerr != LOCKDOWN_E_SUCCESS || !service) {
        return Result<void>::failure(
            ErrorCode::ServiceStartFailed, 
            "Failed to start diagnostics relay service"
        );
    }
    
    diagnostics_relay_client_t raw_diag = nullptr;
    diagnostics_relay_error_t err = diagnostics_relay_client_new(
        device_manager_.device_handle(),
        service.get(),
        &raw_diag
    );
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS || !raw_diag) {
        return Result<void>::failure(
            ErrorCode::ServiceConnectionFailed, 
            "Failed to create diagnostics relay client"
        );
    }
    
    diag_.reset(raw_diag);
    LOG_DEBUG("Diagnostics relay client connected");
    return Result<void>::success();
}

void DiagnosticsClient::disconnect() noexcept {
    diag_.reset();
}

bool DiagnosticsClient::is_connected() const noexcept {
    return static_cast<bool>(diag_);
}

Result<void> DiagnosticsClient::ensure_connected() {
    if (!is_connected()) {
        return connect();
    }
    return Result<void>::success();
}

Result<void> DiagnosticsClient::restart() {
    auto conn = ensure_connected();
    if (!conn) return conn;
    
    LOG_INFO("Sending device restart command...");
    
    diagnostics_relay_error_t err = diagnostics_relay_restart(
        diag_.get(),
        DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT
    );
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS) {
        return Result<void>::failure(
            ErrorCode::DiagnosticsOperationFailed, "Failed to restart device"
        );
    }
    
    LOG_SUCCESS("Restart command sent");
    return Result<void>::success();
}

Result<void> DiagnosticsClient::shutdown() {
    auto conn = ensure_connected();
    if (!conn) return conn;
    
    LOG_INFO("Sending device shutdown command...");
    
    diagnostics_relay_error_t err = diagnostics_relay_shutdown(
        diag_.get(),
        DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT
    );
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS) {
        return Result<void>::failure(
            ErrorCode::DiagnosticsOperationFailed, "Failed to shutdown device"
        );
    }
    
    LOG_SUCCESS("Shutdown command sent");
    return Result<void>::success();
}

Result<void> DiagnosticsClient::sleep() {
    auto conn = ensure_connected();
    if (!conn) return conn;
    
    LOG_INFO("Sending device sleep command...");
    
    diagnostics_relay_error_t err = diagnostics_relay_sleep(diag_.get());
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS) {
        return Result<void>::failure(
            ErrorCode::DiagnosticsOperationFailed, "Failed to put device to sleep"
        );
    }
    
    LOG_SUCCESS("Sleep command sent");
    return Result<void>::success();
}

Result<std::string> DiagnosticsClient::get_battery_info() {
    auto conn = ensure_connected();
    if (!conn) {
        return Result<std::string>::failure(conn.error());
    }
    
    plist_t info = nullptr;
    diagnostics_relay_error_t err = diagnostics_relay_request_diagnostics(
        diag_.get(), "GasGauge", &info
    );
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS || !info) {
        return Result<std::string>::failure(
            ErrorCode::DiagnosticsOperationFailed, "Failed to get battery info"
        );
    }
    
    PlistHandle plist_guard{info};
    
    char* xml = nullptr;
    std::uint32_t length = 0;
    plist_to_xml(info, &xml, &length);
    
    if (!xml) {
        return Result<std::string>::failure(
            ErrorCode::DiagnosticsOperationFailed, 
            "Failed to serialize battery info"
        );
    }
    
    std::string result(xml, length);
    free(xml);
    return Result<std::string>::success(std::move(result));
}

Result<std::string> DiagnosticsClient::get_ioreg_entry(std::string_view key) {
    auto conn = ensure_connected();
    if (!conn) {
        return Result<std::string>::failure(conn.error());
    }
    
    plist_t info = nullptr;
    const std::string key_str{key};
    diagnostics_relay_error_t err = diagnostics_relay_query_ioregistry_entry(
        diag_.get(), key_str.c_str(), "", &info
    );
    
    if (err != DIAGNOSTICS_RELAY_E_SUCCESS || !info) {
        return Result<std::string>::failure(
            ErrorCode::DiagnosticsOperationFailed, "Failed to query IORegistry"
        );
    }
    
    PlistHandle plist_guard{info};
    
    char* xml = nullptr;
    std::uint32_t length = 0;
    plist_to_xml(info, &xml, &length);
    
    if (!xml) {
        return Result<std::string>::failure(
            ErrorCode::DiagnosticsOperationFailed, 
            "Failed to serialize IORegistry entry"
        );
    }
    
    std::string result(xml, length);
    free(xml);
    return Result<std::string>::success(std::move(result));
}

} // namespace a12bypass
