/// @file test_core.cpp
/// @brief Unit tests for core components (Result, Error, types).
/// @copyright MIT License

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "iCloudBypassA12/core/types.hpp"
#include "iCloudBypassA12/core/error.hpp"
#include "iCloudBypassA12/core/result.hpp"
#include "iCloudBypassA12/bypass/config.hpp"

using namespace a12bypass;

// ============================================================================
// Strong Types Tests
// ============================================================================

TEST_CASE("StrongType - Basic operations", "[types][strong_type]") {
    SECTION("Construction and access") {
        Udid udid{"00008030-001234567890002E"};
        REQUIRE(udid.get() == "00008030-001234567890002E");
        REQUIRE(!udid.get().empty());
    }
    
    SECTION("Empty check") {
        Udid empty_udid{""};
        REQUIRE(empty_udid.get().empty());
    }
    
    SECTION("Comparison") {
        Guid guid1{"AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"};
        Guid guid2{"AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"};
        Guid guid3{"11111111-2222-3333-4444-555555555555"};
        
        REQUIRE(guid1 == guid2);
        REQUIRE(guid1 != guid3);
    }
    
    SECTION("Move semantics") {
        ProductType pt1{"iPhone14,5"};
        ProductType pt2{std::move(pt1)};
        REQUIRE(pt2.get() == "iPhone14,5");
    }
}

TEST_CASE("DeviceInfo - Properties", "[types][device_info]") {
    DeviceInfo device{
        .udid = Udid{"00008030-001234567890002E"},
        .product_type = ProductType{"iPhone14,5"},
        .product_version = "17.0",
        .serial_number = SerialNumber{"ABC123"},
        .device_name = "Test iPhone",
        .activation_state = "Unactivated",
        .build_version = "21A5248v",
        .hardware_model = "D63AP",
        .device_class = "iPhone"
    };
    
    SECTION("Activation state") {
        REQUIRE(device.is_activated() == false);
        
        device.activation_state = "Activated";
        REQUIRE(device.is_activated() == true);
    }
    
    SECTION("Normalized product type") {
        REQUIRE(device.normalized_product_type() == "iPhone14-5");
    }
}

// ============================================================================
// Error Tests
// ============================================================================

TEST_CASE("ErrorCategory - String conversion", "[error][category]") {
    REQUIRE(to_string(ErrorCategory::None) == "None");
    REQUIRE(to_string(ErrorCategory::Device) == "Device");
    REQUIRE(to_string(ErrorCategory::Service) == "Service");
    REQUIRE(to_string(ErrorCategory::FileSystem) == "FileSystem");
    REQUIRE(to_string(ErrorCategory::Validation) == "Validation");
    REQUIRE(to_string(ErrorCategory::Timeout) == "Timeout");
    REQUIRE(to_string(ErrorCategory::Cancelled) == "Cancelled");
}

TEST_CASE("ErrorCode - Category mapping", "[error][code]") {
    REQUIRE(category_for(ErrorCode::Success) == ErrorCategory::None);
    REQUIRE(category_for(ErrorCode::DeviceNotFound) == ErrorCategory::Device);
    REQUIRE(category_for(ErrorCode::ServiceStartFailed) == ErrorCategory::Service);
    REQUIRE(category_for(ErrorCode::FileNotFound) == ErrorCategory::FileSystem);
    REQUIRE(category_for(ErrorCode::PayloadGenerationFailed) == ErrorCategory::Payload);
    REQUIRE(category_for(ErrorCode::InvalidGuid) == ErrorCategory::Validation);
    REQUIRE(category_for(ErrorCode::OperationTimeout) == ErrorCategory::Timeout);
    REQUIRE(category_for(ErrorCode::OperationCancelled) == ErrorCategory::Cancelled);
    REQUIRE(category_for(ErrorCode::InternalError) == ErrorCategory::Internal);
}

TEST_CASE("Error - Construction and properties", "[error]") {
    SECTION("Default construction is success") {
        Error err;
        REQUIRE(err.is_success() == true);
        REQUIRE(static_cast<bool>(err) == false);  // false means no error
        REQUIRE(err.code() == ErrorCode::Success);
    }
    
    SECTION("Construction with code and message") {
        Error err{ErrorCode::DeviceNotFound, "No device connected"};
        REQUIRE(err.code() == ErrorCode::DeviceNotFound);
        REQUIRE(err.message() == "No device connected");
        REQUIRE(err.category() == ErrorCategory::Device);
        REQUIRE(static_cast<bool>(err) == true);  // true means has error
    }
    
    SECTION("Construction with just message") {
        Error err{"Something went wrong"};
        REQUIRE(err.code() == ErrorCode::UnknownError);
        REQUIRE(err.message() == "Something went wrong");
    }
    
    SECTION("Factory methods") {
        auto success = Error::success();
        REQUIRE(success.is_success());
        
        auto not_found = Error::device_not_found();
        REQUIRE(not_found.code() == ErrorCode::DeviceNotFound);
        
        auto timeout = Error::timeout("Connection");
        REQUIRE(timeout.code() == ErrorCode::OperationTimeout);
        REQUIRE_THAT(timeout.message(), Catch::Matchers::ContainsSubstring("Connection"));
        
        auto cancelled = Error::cancelled();
        REQUIRE(cancelled.code() == ErrorCode::OperationCancelled);
        
        auto invalid = Error::invalid_guid("bad-guid");
        REQUIRE(invalid.code() == ErrorCode::InvalidGuid);
        REQUIRE_THAT(invalid.message(), Catch::Matchers::ContainsSubstring("bad-guid"));
    }
    
    SECTION("Source location tracking") {
        Error err{ErrorCode::InternalError, "test"};
        REQUIRE(err.location().line() > 0);
        REQUIRE(std::string_view{err.location().file_name()}.find("test_core.cpp") != std::string_view::npos);
    }
}

// ============================================================================
// Result<T> Tests
// ============================================================================

TEST_CASE("Result<T> - Construction", "[result]") {
    SECTION("Value construction") {
        Result<int> r{42};
        REQUIRE(r.has_value());
        REQUIRE(r.value() == 42);
        REQUIRE(static_cast<bool>(r) == true);
    }
    
    SECTION("Error construction") {
        Result<int> r{Error{ErrorCode::InvalidGuid, "bad guid"}};
        REQUIRE(!r.has_value());
        REQUIRE(r.error().code() == ErrorCode::InvalidGuid);
    }
    
    SECTION("Error code + message construction") {
        Result<int> r{ErrorCode::FileNotFound, "missing file"};
        REQUIRE(!r.has_value());
        REQUIRE(r.error().code() == ErrorCode::FileNotFound);
    }
    
    SECTION("Default construction for default-constructible types") {
        Result<std::string> r;
        REQUIRE(r.has_value());
        REQUIRE(r.value().empty());
    }
    
    SECTION("Factory methods") {
        auto success = Result<int>::success(100);
        REQUIRE(success.has_value());
        REQUIRE(success.value() == 100);
        
        auto failure = Result<int>::failure(ErrorCode::InternalError, "oops");
        REQUIRE(!failure.has_value());
        REQUIRE(failure.error().code() == ErrorCode::InternalError);
    }
}

TEST_CASE("Result<T> - Value access", "[result]") {
    Result<std::string> r{"hello"};
    
    SECTION("value() returns reference") {
        REQUIRE(r.value() == "hello");
        r.value() = "world";
        REQUIRE(r.value() == "world");
    }
    
    SECTION("operator* access") {
        REQUIRE(*r == "hello");
    }
    
    SECTION("operator-> access") {
        REQUIRE(r->size() == 5);
    }
    
    SECTION("value_or with value") {
        REQUIRE(r.value_or("default") == "hello");
    }
    
    SECTION("value_or with error") {
        Result<std::string> err{ErrorCode::InternalError, "test"};
        REQUIRE(err.value_or("default") == "default");
    }
}

TEST_CASE("Result<T> - Monadic operations", "[result][monadic]") {
    SECTION("map transforms value") {
        Result<int> r{10};
        auto doubled = r.map([](int x) { return x * 2; });
        REQUIRE(doubled.has_value());
        REQUIRE(doubled.value() == 20);
    }
    
    SECTION("map propagates error") {
        Result<int> r{ErrorCode::InvalidGuid, "bad"};
        auto doubled = r.map([](int x) { return x * 2; });
        REQUIRE(!doubled.has_value());
        REQUIRE(doubled.error().code() == ErrorCode::InvalidGuid);
    }
    
    SECTION("map changes type") {
        Result<int> r{42};
        auto as_string = r.map([](int x) { return std::to_string(x); });
        REQUIRE(as_string.has_value());
        REQUIRE(as_string.value() == "42");
    }
    
    SECTION("and_then chains operations") {
        auto divide = [](int x) -> Result<int> {
            if (x == 0) {
                return Error{ErrorCode::InvalidConfiguration, "division by zero"};
            }
            return 100 / x;
        };
        
        Result<int> r{5};
        auto result = r.and_then(divide);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 20);
    }
    
    SECTION("and_then short-circuits on error") {
        auto divide = [](int x) -> Result<int> {
            return 100 / x;
        };
        
        Result<int> r{ErrorCode::InternalError, "test"};
        auto result = r.and_then(divide);
        REQUIRE(!result.has_value());
        REQUIRE(result.error().code() == ErrorCode::InternalError);
    }
    
    SECTION("or_else provides alternative") {
        Result<int> r{ErrorCode::FileNotFound, "not found"};
        auto recovered = r.or_else([](const Error&) -> Result<int> {
            return 0;  // default value
        });
        REQUIRE(recovered.has_value());
        REQUIRE(recovered.value() == 0);
    }
    
    SECTION("or_else not called on success") {
        Result<int> r{42};
        bool called = false;
        auto same = r.or_else([&](const Error&) -> Result<int> {
            called = true;
            return 0;
        });
        REQUIRE(same.has_value());
        REQUIRE(same.value() == 42);
        REQUIRE(!called);
    }
    
    SECTION("map_error transforms error") {
        Result<int> r{ErrorCode::FileNotFound, "file.txt"};
        auto transformed = r.map_error([](const Error& e) {
            return Error{ErrorCode::InternalError, "wrapped: " + e.message()};
        });
        REQUIRE(!transformed.has_value());
        REQUIRE(transformed.error().code() == ErrorCode::InternalError);
        REQUIRE_THAT(transformed.error().message(), Catch::Matchers::ContainsSubstring("wrapped"));
    }
}

TEST_CASE("Result<T> - Move semantics", "[result]") {
    SECTION("Move construct from value") {
        auto make_result = []() -> Result<std::string> {
            return std::string{"moved string"};
        };
        
        auto r = make_result();
        REQUIRE(r.has_value());
        REQUIRE(r.value() == "moved string");
    }
    
    SECTION("Move value out") {
        Result<std::string> r{"content"};
        std::string moved = std::move(r).value();
        REQUIRE(moved == "content");
    }
}

// ============================================================================
// Result<void> Tests
// ============================================================================

TEST_CASE("Result<void> - Construction", "[result][void]") {
    SECTION("Default construction is success") {
        Result<void> r;
        REQUIRE(r.has_value());
        REQUIRE(static_cast<bool>(r) == true);
    }
    
    SECTION("Error construction") {
        Result<void> r{Error{ErrorCode::OperationFailed, "failed"}};
        REQUIRE(!r.has_value());
        REQUIRE(r.error().code() == ErrorCode::OperationFailed);
    }
    
    SECTION("Factory methods") {
        auto success = Result<void>::success();
        REQUIRE(success.has_value());
        
        auto failure = Result<void>::failure(ErrorCode::OperationCancelled);
        REQUIRE(!failure.has_value());
    }
}

TEST_CASE("Result<void> - and_then chains", "[result][void]") {
    bool executed = false;
    
    SECTION("Executes on success") {
        Result<void> r;
        auto result = r.and_then([&]() -> Result<int> {
            executed = true;
            return 42;
        });
        REQUIRE(executed);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 42);
    }
    
    SECTION("Skipped on error") {
        Result<void> r{ErrorCode::FileNotFound, "test"};
        auto result = r.and_then([&]() -> Result<int> {
            executed = true;
            return 42;
        });
        REQUIRE(!executed);
        REQUIRE(!result.has_value());
    }
}

// ============================================================================
// BypassStage Tests
// ============================================================================

TEST_CASE("BypassStage - String conversion", "[config][stage]") {
    REQUIRE(to_string(BypassStage::Init) == "Initializing");
    REQUIRE(to_string(BypassStage::DetectDevice) == "Detecting Device");
    REQUIRE(to_string(BypassStage::ExtractGuid) == "Extracting GUID");
    REQUIRE(to_string(BypassStage::Complete) == "Complete");
    REQUIRE(to_string(BypassStage::Failed) == "Failed");
}

TEST_CASE("BypassStage - Progress percentage", "[config][stage]") {
    REQUIRE(progress_for(BypassStage::Init) == 0);
    REQUIRE(progress_for(BypassStage::DetectDevice) == 5);
    REQUIRE(progress_for(BypassStage::Complete) == 100);
    REQUIRE(progress_for(BypassStage::Failed) == 0);
    
    // Ensure monotonic progress
    int prev = 0;
    for (auto stage : {
        BypassStage::Init,
        BypassStage::DetectDevice,
        BypassStage::ExtractGuid,
        BypassStage::GeneratePayloads,
        BypassStage::CleanDevice,
        BypassStage::UploadPayload,
        BypassStage::Stage1Reboot,
        BypassStage::Stage1Transfer,
        BypassStage::Stage2Reboot,
        BypassStage::Stage2Transfer,
        BypassStage::FinalReboot,
        BypassStage::Complete
    }) {
        int progress = progress_for(stage);
        REQUIRE(progress >= prev);
        prev = progress;
    }
}

// ============================================================================
// BypassConfig Tests
// ============================================================================

TEST_CASE("BypassConfig - Validation", "[config]") {
    BypassConfig config;
    
    SECTION("Default config is valid") {
        REQUIRE(config.validate() == true);
    }
    
    SECTION("Invalid max attempts") {
        config.max_guid_attempts = 0;
        REQUIRE(config.validate() == false);
    }
    
    SECTION("Invalid timeout") {
        config.reboot_timeout = chrono::seconds{10};  // Too short
        REQUIRE(config.validate() == false);
    }
    
    SECTION("Empty preset guid is invalid") {
        config.preset_guid = Guid{""};
        REQUIRE(config.validate() == false);
    }
    
    SECTION("Valid preset guid") {
        config.preset_guid = Guid{"AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"};
        REQUIRE(config.validate() == true);
    }
}
