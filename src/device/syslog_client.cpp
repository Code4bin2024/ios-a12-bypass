/// @file syslog_client.cpp
/// @brief Syslog client implementation.
/// @copyright MIT License

#include "iCloudBypassA12/device/syslog_client.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <chrono>

namespace a12bypass {

SyslogClient::SyslogClient(DeviceManager& device_manager)
    : device_manager_(device_manager)
{}

SyslogClient::~SyslogClient() {
    stop();
    disconnect();
}

Result<void> SyslogClient::connect() {
    if (!device_manager_.is_connected()) {
        return Result<void>::failure(
            ErrorCode::DeviceNotConnected, "Device not connected"
        );
    }
    
    disconnect();
    
    ServiceDescriptorHandle service;
    lockdownd_error_t lerr = lockdownd_start_service(
        device_manager_.lockdown_handle(),
        "com.apple.syslog_relay",
        service.addressof()
    );
    
    if (lerr != LOCKDOWN_E_SUCCESS || !service) {
        return Result<void>::failure(
            ErrorCode::ServiceStartFailed, 
            "Failed to start syslog relay service"
        );
    }
    
    syslog_relay_client_t raw_syslog = nullptr;
    syslog_relay_error_t err = syslog_relay_client_new(
        device_manager_.device_handle(),
        service.get(),
        &raw_syslog
    );
    
    if (err != SYSLOG_RELAY_E_SUCCESS || !raw_syslog) {
        return Result<void>::failure(
            ErrorCode::ServiceConnectionFailed, 
            "Failed to create syslog relay client"
        );
    }
    
    syslog_.reset(raw_syslog);
    LOG_DEBUG("Syslog relay client connected");
    return Result<void>::success();
}

void SyslogClient::disconnect() noexcept {
    stop();
    syslog_.reset();
}

bool SyslogClient::is_connected() const noexcept {
    return static_cast<bool>(syslog_);
}

Result<void> SyslogClient::start(SyslogCallback callback) {
    if (!is_connected()) {
        auto conn = connect();
        if (!conn) return conn;
    }
    
    if (running_) {
        return Result<void>::failure(
            ErrorCode::OperationFailed, 
            "Syslog collection already running"
        );
    }
    
    callback_ = std::move(callback);
    running_ = true;
    line_buffer_.clear();
    
    syslog_relay_error_t err = syslog_relay_start_capture(
        syslog_.get(), 
        syslog_callback, 
        this
    );
    
    if (err != SYSLOG_RELAY_E_SUCCESS) {
        running_ = false;
        return Result<void>::failure(
            ErrorCode::SyslogOperationFailed, 
            "Failed to start syslog capture"
        );
    }
    
    LOG_DEBUG("Syslog capture started");
    return Result<void>::success();
}

void SyslogClient::stop() noexcept {
    if (running_) {
        running_ = false;
        if (syslog_) {
            syslog_relay_stop_capture(syslog_.get());
        }
    }
}

bool SyslogClient::is_running() const noexcept {
    return running_;
}

void SyslogClient::syslog_callback(char c, void* user_data) {
    auto* self = static_cast<SyslogClient*>(user_data);
    
    if (c == '\n' || c == '\0') {
        if (!self->line_buffer_.empty()) {
            {
                std::lock_guard lock(self->logs_mutex_);
                self->logs_.push_back(self->line_buffer_);
            }
            
            if (self->callback_) {
                self->callback_(self->line_buffer_);
            }
            
            self->line_buffer_.clear();
        }
    } else {
        self->line_buffer_ += c;
    }
}

Result<std::string> SyslogClient::collect_and_search(
    chrono::seconds duration,
    std::string_view search_pattern,
    std::string_view process_filter
) {
    LOG_INFO("Collecting syslogs for ", duration.count(), " seconds...");
    
    std::string found_line;
    std::atomic<bool> found{false};
    
    const std::string pattern_str{search_pattern};
    const std::string filter_str{process_filter};
    
    auto result = start([&](std::string_view line) {
        if (found) return;
        
        // Check process filter
        if (!filter_str.empty() && 
            line.find(filter_str) == std::string_view::npos) {
            return;
        }
        
        // Check search pattern
        if (line.find(pattern_str) != std::string_view::npos) {
            found_line = std::string(line);
            found = true;
            LOG_DEBUG("Found matching log line");
        }
    });
    
    if (!result) {
        return Result<std::string>::failure(result.error());
    }
    
    // Wait for duration or until found
    const auto start_time = chrono::steady_clock::now();
    while (!found && running_) {
        auto elapsed = chrono::steady_clock::now() - start_time;
        if (elapsed >= duration) {
            break;
        }
        std::this_thread::sleep_for(100ms);
    }
    
    stop();
    
    if (found) {
        return Result<std::string>::success(std::move(found_line));
    }
    
    return Result<std::string>::failure(
        ErrorCode::OperationFailed, "Pattern not found in syslog"
    );
}

Result<std::vector<std::string>> SyslogClient::collect(chrono::seconds duration) {
    LOG_INFO("Collecting syslogs for ", duration.count(), " seconds...");
    
    clear_logs();
    
    auto result = start([](std::string_view) {
        // Just collect, callback does nothing special
    });
    
    if (!result) {
        return Result<std::vector<std::string>>::failure(result.error());
    }
    
    std::this_thread::sleep_for(duration);
    stop();
    
    std::lock_guard lock(logs_mutex_);
    return Result<std::vector<std::string>>::success(logs_);
}

const std::vector<std::string>& SyslogClient::logs() const {
    std::lock_guard lock(logs_mutex_);
    return logs_;
}

void SyslogClient::clear_logs() {
    std::lock_guard lock(logs_mutex_);
    logs_.clear();
}

} // namespace a12bypass
