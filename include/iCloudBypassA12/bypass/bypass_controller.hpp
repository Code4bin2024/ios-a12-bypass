/// @file bypass_controller.hpp
/// @brief Main orchestration controller for iOS activation bypass procedure
/// @copyright MIT License - Educational & Research Use Only

#pragma once

#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/bypass/config.hpp"
#include "iCloudBypassA12/device/device_manager.hpp"
#include "iCloudBypassA12/device/afc_client.hpp"
#include "iCloudBypassA12/device/diagnostics_client.hpp"
#include "iCloudBypassA12/device/syslog_client.hpp"
#include "iCloudBypassA12/payload/payload_generator.hpp"
#include <atomic>
#include <memory>

namespace a12bypass {

/// Main orchestrator for the bypass procedure
class BypassController {
public:
    /// Construct with configuration
    explicit BypassController(const BypassConfig& config);
    ~BypassController();
    
    // Not copyable or movable
    BypassController(const BypassController&) = delete;
    BypassController& operator=(const BypassController&) = delete;
    BypassController(BypassController&&) = delete;
    BypassController& operator=(BypassController&&) = delete;
    
    /// Run the complete bypass procedure
    [[nodiscard]] Result<void> run();
    
    /// Cancel the bypass (can be called from another thread)
    void cancel() noexcept;
    
    /// Get current stage
    [[nodiscard]] BypassStage current_stage() const noexcept;
    
    /// Get current stage info
    [[nodiscard]] StageInfo current_stage_info() const noexcept;
    
    /// Set progress callback
    void set_progress_callback(ProgressCallback callback);
    
    // ========================================================================
    // Individual Steps (for manual control)
    // ========================================================================
    
    /// Detect connected device
    [[nodiscard]] Result<DeviceInfo> detect_device();
    
    /// Extract GUID automatically via syslog
    [[nodiscard]] Result<Guid> extract_guid();
    
    /// Extract GUID via manual input
    [[nodiscard]] Result<Guid> extract_guid_manual();
    
    /// Generate all payload files
    [[nodiscard]] Result<PayloadFiles> generate_payloads(const Guid& guid);
    
    /// Upload payload to device
    [[nodiscard]] Result<void> upload_payload(const PayloadFiles& files);
    
    /// Execute bypass stages
    [[nodiscard]] Result<void> execute_stages();

private:
    BypassConfig config_;
    
    // Device components
    std::unique_ptr<DeviceManager> device_manager_;
    std::unique_ptr<AfcClient> afc_client_;
    std::unique_ptr<DiagnosticsClient> diag_client_;
    std::unique_ptr<SyslogClient> syslog_client_;
    std::unique_ptr<PayloadGenerator> payload_generator_;
    
    // State
    std::atomic<BypassStage> current_stage_{BypassStage::Init};
    std::atomic<bool> cancelled_{false};
    DeviceInfo device_info_;
    Guid guid_;
    PayloadFiles payload_files_;
    
    // Progress callback
    ProgressCallback progress_callback_;
    
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    void set_stage(BypassStage stage);
    void report_progress(int percent, std::string_view message);
    
    [[nodiscard]] Result<void> wait_for_device_reconnect();
    [[nodiscard]] Result<void> reboot_and_wait();
    [[nodiscard]] Result<void> clean_device_files();
    
    // Stage implementations
    [[nodiscard]] Result<void> run_stage1();
    [[nodiscard]] Result<void> run_stage2();
    [[nodiscard]] Result<void> run_final_reboot();
    
    // GUID extraction with retries
    [[nodiscard]] Result<Guid> extract_guid_with_retries(int max_attempts);
};

} // namespace a12bypass
