/// @file bypass_controller.cpp
/// @brief Main activation bypass orchestration controller implementation
/// @copyright MIT License - Educational & Research Use Only
/// @warning This software is for educational purposes. Use responsibly and ethically.

#include "iCloudBypassA12/bypass/bypass_controller.hpp"
#include "iCloudBypassA12/utils/guid_extractor.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <iostream>
#include <thread>

namespace a12bypass {

BypassController::BypassController(const BypassConfig& config)
    : config_(config)
{
    Logger::instance().set_verbose(config.verbose);
    
    // Determine resources path
    if (config_.resources_path.empty()) {
        config_.resources_path = config_.effective_resources_path();
    }
}

BypassController::~BypassController() {
    cancel();
}

Result<void> BypassController::run() {
    LOG_STEP("Professional iOS A12+ Activation Solution - Initiating Unlock Sequence");
    LOG_INFO("Starting bypass procedure...");
    
    cancelled_ = false;
    
    // Phase 1: Detect device
    set_stage(BypassStage::DetectDevice);
    auto device_result = detect_device();
    if (!device_result) {
        set_stage(BypassStage::Failed);
        return Result<void>::failure(device_result.error());
    }
    device_info_ = device_result.value();
    
    if (cancelled_) return Result<void>::failure(Error::cancelled());
    
    // Phase 2: Get GUID
    set_stage(BypassStage::ExtractGuid);
    if (config_.preset_guid) {
        guid_ = *config_.preset_guid;
        if (!GuidExtractor::validate_guid(guid_.get())) {
            set_stage(BypassStage::Failed);
            return Result<void>::failure(Error::invalid_guid(guid_.get()));
        }
        LOG_INFO("Using preset GUID: ", guid_.get());
    } else if (config_.auto_mode) {
        auto guid_result = extract_guid_with_retries(config_.max_guid_attempts);
        if (!guid_result) {
            set_stage(BypassStage::Failed);
            return Result<void>::failure(guid_result.error());
        }
        guid_ = guid_result.value();
    } else {
        auto guid_result = extract_guid_manual();
        if (!guid_result) {
            set_stage(BypassStage::Failed);
            return Result<void>::failure(guid_result.error());
        }
        guid_ = guid_result.value();
    }
    
    if (cancelled_) return Result<void>::failure(Error::cancelled());
    
    // Phase 3: Generate payloads
    set_stage(BypassStage::GeneratePayloads);
    auto payload_result = generate_payloads(guid_);
    if (!payload_result) {
        set_stage(BypassStage::Failed);
        return Result<void>::failure(payload_result.error());
    }
    payload_files_ = payload_result.value();
    
    if (cancelled_) return Result<void>::failure(Error::cancelled());
    
    // Phase 4: Clean device and upload payload
    set_stage(BypassStage::CleanDevice);
    auto clean_result = clean_device_files();
    if (!clean_result) {
        LOG_WARN("Failed to clean some files: ", clean_result.error().message());
    }
    
    set_stage(BypassStage::UploadPayload);
    auto upload_result = upload_payload(payload_files_);
    if (!upload_result) {
        set_stage(BypassStage::Failed);
        return Result<void>::failure(upload_result.error());
    }
    
    if (cancelled_) return Result<void>::failure(Error::cancelled());
    
    // Phase 5: Execute bypass stages
    auto stages_result = execute_stages();
    if (!stages_result) {
        set_stage(BypassStage::Failed);
        return Result<void>::failure(stages_result.error());
    }
    
    set_stage(BypassStage::Complete);
    
    std::cout << "\n";
    LOG_SUCCESS("ACTIVATION BYPASS COMPLETE!");
    LOG_INFO("GUID: ", guid_.get());
    LOG_INFO("Device: ", device_info_.product_type.get());
    std::cout << "\n";
    
    return Result<void>::success();
}

void BypassController::cancel() noexcept {
    cancelled_ = true;
}

BypassStage BypassController::current_stage() const noexcept {
    return current_stage_.load(std::memory_order_relaxed);
}

StageInfo BypassController::current_stage_info() const noexcept {
    return StageInfo::from(current_stage());
}

void BypassController::set_progress_callback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
}

Result<DeviceInfo> BypassController::detect_device() {
    LOG_STEP("Device Detection");
    
    if (!device_manager_) {
        device_manager_ = std::make_unique<DeviceManager>();
    }
    
    return device_manager_->detect_device();
}

Result<Guid> BypassController::extract_guid() {
    LOG_STEP("GUID Extraction");
    
    if (!syslog_client_) {
        syslog_client_ = std::make_unique<SyslogClient>(*device_manager_);
    }
    
    // Reboot device first to trigger bookassetd
    auto reboot_result = reboot_and_wait();
    if (!reboot_result) {
        return Result<Guid>::failure(reboot_result.error());
    }
    
    // Collect syslogs and search for BLDatabaseManager path
    LOG_INFO("Collecting syslogs...");
    auto result = syslog_client_->collect_and_search(
        config_.syslog_timeout,
        "BLDatabaseManager.sqlite",
        "bookassetd"
    );
    
    if (!result) {
        return Result<Guid>::failure(
            ErrorCode::OperationFailed,
            "GUID not found in syslogs: " + result.error().message()
        );
    }
    
    // Extract GUID from the found line
    auto guid = GuidExtractor::extract_from_line(result.value());
    if (!guid) {
        return Result<Guid>::failure(
            ErrorCode::OperationFailed,
            "Failed to extract GUID from log line"
        );
    }
    
    LOG_SUCCESS("Extracted GUID: ", *guid);
    return Result<Guid>::success(Guid{*guid});
}

Result<Guid> BypassController::extract_guid_manual() {
    std::cout << "\n" << ansi::yellow 
              << "Enter SystemGroup GUID manually" << ansi::reset << "\n";
    std::cout << "Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX\n";
    
    while (true) {
        std::cout << ansi::blue << "> GUID: " << ansi::reset;
        std::string input;
        std::getline(std::cin, input);
        
        // Normalize
        std::string guid = GuidExtractor::normalize(input);
        
        if (GuidExtractor::validate_guid(guid)) {
            return Result<Guid>::success(Guid{guid});
        }
        
        std::cout << ansi::red << "Invalid GUID format. Try again.\n" << ansi::reset;
    }
}

Result<Guid> BypassController::extract_guid_with_retries(int max_attempts) {
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        LOG_INFO("GUID extraction attempt ", attempt, "/", max_attempts);
        
        auto result = extract_guid();
        if (result) {
            return result;
        }
        
        if (attempt < max_attempts) {
            LOG_WARN("Attempt failed: ", result.error().message());
            LOG_INFO("Retrying...");
        }
    }
    
    return Result<Guid>::failure(
        ErrorCode::OperationFailed,
        "GUID extraction failed after " + std::to_string(max_attempts) + " attempts"
    );
}

Result<PayloadFiles> BypassController::generate_payloads(const Guid& guid) {
    LOG_STEP("Payload Generation");
    
    if (!payload_generator_) {
        payload_generator_ = std::make_unique<PayloadGenerator>(
            config_.resources_path
        );
    }
    
    // Check device support
    if (!payload_generator_->is_device_supported(device_info_.product_type.get())) {
        return Result<PayloadFiles>::failure(
            ErrorCode::DeviceNotSupported,
            "Device not supported: " + device_info_.product_type.get() + 
            ". No MobileGestalt plist found."
        );
    }
    
    // Generate to temp directory
    auto temp_dir = fs::temp_directory_path() / "a12bypass_payloads";
    
    return payload_generator_->generate(
        device_info_.product_type.get(), 
        guid.get(), 
        temp_dir
    );
}

Result<void> BypassController::upload_payload(const PayloadFiles& files) {
    LOG_STEP("Payload Upload");
    
    if (!afc_client_) {
        afc_client_ = std::make_unique<AfcClient>(*device_manager_);
    }
    
    auto connect_result = afc_client_->connect();
    if (!connect_result) {
        return connect_result;
    }
    
    // Upload downloads.28.sqlitedb
    auto push_result = afc_client_->push_file(
        files.downloads_db, 
        paths::device::downloads_db
    );
    if (!push_result) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed,
            "Failed to upload payload: " + push_result.error().message()
        );
    }
    
    // Verify upload
    auto exists = afc_client_->file_exists(paths::device::downloads_db);
    if (!exists || !exists.value()) {
        return Result<void>::failure(
            ErrorCode::AfcOperationFailed,
            "Payload verification failed"
        );
    }
    
    LOG_SUCCESS("Payload uploaded successfully");
    return Result<void>::success();
}

Result<void> BypassController::execute_stages() {
    // Stage 1
    auto result = run_stage1();
    if (!result) return result;
    
    // Stage 2
    result = run_stage2();
    if (!result) return result;
    
    // Final reboot
    result = run_final_reboot();
    if (!result) return result;
    
    return Result<void>::success();
}

void BypassController::set_stage(BypassStage stage) {
    current_stage_.store(stage, std::memory_order_relaxed);
    if (progress_callback_) {
        auto info = current_stage_info();
        progress_callback_(info.name, info.progress_percent, info.description);
    }
}

void BypassController::report_progress(int percent, std::string_view message) {
    if (progress_callback_) {
        auto info = current_stage_info();
        progress_callback_(info.name, percent, message);
    }
}

Result<void> BypassController::wait_for_device_reconnect() {
    LOG_INFO("Waiting for device to reconnect...");
    return device_manager_->wait_for_device(config_.reboot_timeout);
}

Result<void> BypassController::reboot_and_wait() {
    if (!diag_client_) {
        diag_client_ = std::make_unique<DiagnosticsClient>(*device_manager_);
    }
    
    auto reboot_result = diag_client_->restart();
    if (!reboot_result) {
        LOG_WARN("Soft reboot failed, please reboot device manually");
        std::cout << "Press Enter after rebooting...";
        std::cin.get();
    }
    
    // Disconnect and wait for reconnection
    device_manager_->disconnect();
    
    auto wait_result = wait_for_device_reconnect();
    if (!wait_result) {
        return wait_result;
    }
    
    // Reconnect services
    auto connect_result = device_manager_->connect(device_info_.udid.get());
    if (!connect_result) {
        return Result<void>::failure(
            ErrorCode::DeviceConnectionFailed,
            "Failed to reconnect after reboot"
        );
    }
    
    // Wait for boot to complete
    std::this_thread::sleep_for(8s);
    
    return Result<void>::success();
}

Result<void> BypassController::clean_device_files() {
    if (!afc_client_) {
        afc_client_ = std::make_unique<AfcClient>(*device_manager_);
    }
    
    return afc_client_->clean_bypass_files();
}

Result<void> BypassController::run_stage1() {
    set_stage(BypassStage::Stage1Reboot);
    LOG_STEP("Stage 1: Trigger itunesstored");
    
    auto reboot_result = reboot_and_wait();
    if (!reboot_result) {
        return reboot_result;
    }
    
    // Wait for itunesstored to process
    LOG_INFO("Waiting 30s for itunesstored...");
    std::this_thread::sleep_for(30s);
    
    set_stage(BypassStage::Stage1Transfer);
    
    // Reconnect AFC
    if (!afc_client_) {
        afc_client_ = std::make_unique<AfcClient>(*device_manager_);
    }
    (void)afc_client_->connect();
    
    // Try to copy iTunesMetadata.plist to /Books/
    auto temp_file = fs::temp_directory_path() / "tmp_metadata.plist";
    
    auto pull_result = afc_client_->pull_file(
        paths::device::itunes_metadata, 
        temp_file
    );
    if (pull_result) {
        auto push_result = afc_client_->push_file(
            temp_file, 
            paths::device::books_metadata
        );
        if (push_result) {
            LOG_SUCCESS("Copied iTunesMetadata.plist to /Books/");
        } else {
            LOG_WARN("Failed to push to /Books/: ", push_result.error().message());
        }
        fs::remove(temp_file);
    } else {
        LOG_WARN("iTunesMetadata.plist not found yet");
    }
    
    return Result<void>::success();
}

Result<void> BypassController::run_stage2() {
    set_stage(BypassStage::Stage2Reboot);
    LOG_STEP("Stage 2: Trigger bookassetd");
    
    std::this_thread::sleep_for(5s);
    
    auto reboot_result = reboot_and_wait();
    if (!reboot_result) {
        return reboot_result;
    }
    
    set_stage(BypassStage::Stage2Transfer);
    
    // Reconnect AFC
    (void)afc_client_->connect();
    
    // Copy back from /Books/ to /iTunes_Control/
    auto temp_file = fs::temp_directory_path() / "tmp_metadata.plist";
    
    auto pull_result = afc_client_->pull_file(
        paths::device::books_metadata, 
        temp_file
    );
    if (pull_result) {
        auto push_result = afc_client_->push_file(
            temp_file, 
            paths::device::itunes_metadata
        );
        if (push_result) {
            LOG_SUCCESS("Restored iTunesMetadata.plist");
        } else {
            LOG_WARN("Failed to restore: ", push_result.error().message());
        }
        fs::remove(temp_file);
    } else {
        LOG_WARN("/Books/iTunesMetadata.plist not found");
    }
    
    // Wait for bookassetd to download and extract asset
    LOG_INFO("Waiting 35s for bookassetd...");
    std::this_thread::sleep_for(35s);
    
    return Result<void>::success();
}

Result<void> BypassController::run_final_reboot() {
    set_stage(BypassStage::FinalReboot);
    LOG_STEP("Final Reboot");
    
    auto reboot_result = reboot_and_wait();
    if (!reboot_result) {
        return reboot_result;
    }
    
    return Result<void>::success();
}

} // namespace a12bypass
