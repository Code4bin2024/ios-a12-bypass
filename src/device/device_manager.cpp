/// @file device_manager.cpp
/// @brief iOS device management implementation.
/// @copyright MIT License

#include "iCloudBypassA12/device/device_manager.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <iostream>
#include <thread>

namespace a12bypass {

Result<std::vector<std::string>> DeviceManager::list_devices() const {
    char** devices = nullptr;
    int count = 0;
    
    idevice_error_t err = idevice_get_device_list(&devices, &count);
    
    if (err == IDEVICE_E_NO_DEVICE) {
        return Result<std::vector<std::string>>::success({});
    }
    
    if (err != IDEVICE_E_SUCCESS) {
        return Result<std::vector<std::string>>::failure(
            ErrorCode::DeviceNotFound, "Failed to enumerate devices"
        );
    }
    
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));
    
    for (int i = 0; i < count; ++i) {
        result.emplace_back(devices[i]);
    }
    
    idevice_device_list_free(devices);
    return Result<std::vector<std::string>>::success(std::move(result));
}

Result<DeviceInfo> DeviceManager::detect_device(std::string_view udid) {
    auto connect_result = connect(udid);
    if (!connect_result) {
        return Result<DeviceInfo>::failure(connect_result.error());
    }
    
    return Result<DeviceInfo>::success(device_info_);
}

Result<void> DeviceManager::connect(std::string_view udid) {
    disconnect();
    
    LOG_INFO("Connecting to device...");
    
    idevice_t raw_device = nullptr;
    idevice_error_t err;
    
    if (udid.empty()) {
        err = idevice_new(&raw_device, nullptr);
    } else {
        err = idevice_new(&raw_device, std::string(udid).c_str());
    }
    
    if (err != IDEVICE_E_SUCCESS || !raw_device) {
        return Result<void>::failure(
            ErrorCode::DeviceConnectionFailed, "Failed to connect to device"
        );
    }
    
    device_.reset(raw_device);
    
    // Get UDID
    char* device_udid = nullptr;
    if (idevice_get_udid(device_.get(), &device_udid) == IDEVICE_E_SUCCESS && device_udid) {
        device_info_.udid = Udid{device_udid};
        free(device_udid);
    }
    
    // Start lockdown service
    auto lockdown_result = start_lockdown();
    if (!lockdown_result) {
        disconnect();
        return lockdown_result;
    }
    
    // Read device information
    populate_device_info();
    
    LOG_SUCCESS("Connected to ", device_info_.device_name, 
                " (", device_info_.product_type.get(), 
                ", iOS ", device_info_.product_version, ")");
    
    if (device_info_.is_activated()) {
        LOG_WARN("Device is already activated");
    }
    
    return Result<void>::success();
}

void DeviceManager::disconnect() noexcept {
    lockdown_.reset();
    device_.reset();
    device_info_ = DeviceInfo{};
}

bool DeviceManager::is_connected() const noexcept {
    return device_ && lockdown_;
}

Result<void> DeviceManager::wait_for_device(chrono::seconds timeout) {
    LOG_INFO("Waiting for device to connect...");
    
    const auto start = chrono::steady_clock::now();
    int dots = 0;
    
    while (true) {
        auto elapsed = chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            std::cout << std::endl;
            return Result<void>::failure(Error::timeout("Device connection"));
        }
        
        auto devices = list_devices();
        if (devices && !devices.value().empty()) {
            std::cout << std::endl;
            return connect(devices.value()[0]);
        }
        
        // Print progress dots
        std::cout << "." << std::flush;
        if (++dots % 60 == 0) {
            std::cout << std::endl;
        }
        
        std::this_thread::sleep_for(3s);
    }
}

Result<void> DeviceManager::start_lockdown() {
    lockdownd_client_t raw_lockdown = nullptr;
    
    lockdownd_error_t err = lockdownd_client_new_with_handshake(
        device_.get(),
        &raw_lockdown,
        "a12bypass"
    );
    
    if (err != LOCKDOWN_E_SUCCESS || !raw_lockdown) {
        return Result<void>::failure(
            ErrorCode::LockdownFailed, "Failed to start lockdown service"
        );
    }
    
    lockdown_.reset(raw_lockdown);
    return Result<void>::success();
}

std::optional<std::string> DeviceManager::get_lockdown_value(
    const char* domain,
    const char* key
) const {
    if (!lockdown_) {
        return std::nullopt;
    }
    
    plist_t value = nullptr;
    lockdownd_error_t err = lockdownd_get_value(lockdown_.get(), domain, key, &value);
    
    if (err != LOCKDOWN_E_SUCCESS || !value) {
        return std::nullopt;
    }
    
    PlistHandle plist_guard{value};
    
    std::string result;
    if (plist_get_node_type(value) == PLIST_STRING) {
        char* str = nullptr;
        plist_get_string_val(value, &str);
        if (str) {
            result = str;
            free(str);
        }
    }
    
    return result.empty() ? std::nullopt : std::make_optional(std::move(result));
}

void DeviceManager::populate_device_info() {
    auto get_or_unknown = [this](const char* key) {
        return get_lockdown_value(nullptr, key).value_or("Unknown");
    };
    
    device_info_.product_type = ProductType{get_or_unknown("ProductType")};
    device_info_.product_version = get_or_unknown("ProductVersion");
    device_info_.serial_number = SerialNumber{get_or_unknown("SerialNumber")};
    device_info_.device_name = get_or_unknown("DeviceName");
    device_info_.activation_state = get_or_unknown("ActivationState");
    device_info_.build_version = get_or_unknown("BuildVersion");
    device_info_.hardware_model = get_or_unknown("HardwareModel");
    device_info_.device_class = get_or_unknown("DeviceClass");
}

void DeviceManager::set_event_callback(DeviceEventCallback callback) {
    event_callback_ = std::move(callback);
}

} // namespace a12bypass
