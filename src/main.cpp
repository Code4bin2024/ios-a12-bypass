/// @file main.cpp
/// @brief CLI entry point for iOS A12+ Bypass Toolkit
/// @copyright MIT License - Educational & Research Use Only
/// @warning This software is for educational and research purposes ONLY.
///          Unauthorized use on devices you don't own is illegal.
///          Use responsibly and ethically.

#include "iCloudBypassA12/bypass/bypass_controller.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include "iCloudBypassA12/core/types.hpp"
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace a12bypass;

// ============================================================================
// Global State
// ============================================================================

namespace {
    /// Global controller instance for signal handling
    std::unique_ptr<BypassController> g_controller;
    
    /// Color output enabled
    bool g_colors_enabled = true;
    
    /// Quiet mode
    bool g_quiet_mode = false;
    
    /// Signal handler for graceful cancellation
    void signal_handler(int /*sig*/) {
        if (!g_quiet_mode) {
            std::cout << "\n";
            if (g_colors_enabled) {
                std::cout << ansi::yellow << "Interrupt received, cancelling..." 
                          << ansi::reset << "\n";
            } else {
                std::cout << "Interrupt received, cancelling...\n";
            }
        }
        if (g_controller) {
            g_controller->cancel();
        }
    }
}

// ============================================================================
// CLI Interface
// ============================================================================

/// Print ASCII banner
void print_banner() {
    if (g_quiet_mode) return;
    
    std::cout << R"(
    ╔═════════════════════════════════════════════════════════╗
    ║                                                         ║
    ║     █████╗ ██╗██████╗     ██╗   ██╗███╗   ██╗██╗        ║
    ║    ██╔══██╗███║╚════██╗    ██║   ██║████╗  ██║██║       ║
    ║    ███████║╚██║ █████╔╝    ██║   ██║██╔██╗ ██║██║       ║
    ║    ██╔══██║ ██║██╔═══╝     ██║   ██║██║╚██╗██║██║       ║
    ║    ██║  ██║ ██║███████╗    ╚██████╔╝██║ ╚████║███████╗  ║
    ║    ╚═╝  ╚═╝ ╚═╝╚══════╝     ╚═════╝ ╚═╝  ╚═══╝╚══════╝  ║
    ║                                                         ║
    ╚═════════════════════════════════════════════════════════╝
)" << "\n";
    
    if (g_colors_enabled) {
        std::cout << ansi::cyan << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" 
                  << ansi::reset << "\n";
        std::cout << ansi::bold << ansi::green << "  Professional iOS Activation Solution for A12+ Devices" 
                  << ansi::reset << "\n";
        std::cout << ansi::cyan << "  Advanced Unlock Framework | v" << version::string 
                  << ansi::reset << "\n";
        std::cout << ansi::dim << "  Built with Modern C++20 Architecture" 
                  << ansi::reset << "\n";
        std::cout << ansi::cyan << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" 
                  << ansi::reset << "\n\n";
    } else {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "  Professional iOS Activation Solution for A12+ Devices\n";
        std::cout << "  Advanced Unlock Framework | v" << version::string << "\n";
        std::cout << "  Built with Modern C++20 Architecture\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
}

/// Print usage information
void print_usage(std::string_view program) {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Usage: " << program << " [options]\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "🎯 OPERATION MODES:\n";
    std::cout << "  -a, --auto             Automated execution (skip prompts, detect GUID)\n";
    std::cout << "  -n, --dry-run          Simulation mode (preview without changes)\n";
    std::cout << "  -l, --list             Display all compatible device models\n";
    std::cout << "  -i, --info             Show detailed connected device information\n";
    std::cout << "\n";
    
    std::cout << "⚙️  CONFIGURATION:\n";
    std::cout << "  -g, --guid GUID        Provide pre-extracted SystemGroup GUID\n";
    std::cout << "  -o, --output DIR       Specify payload generation directory\n";
    std::cout << "  -t, --timeout SECS     Device reconnection timeout (default: 300s)\n";
    std::cout << "  -r, --retries N        Maximum GUID extraction attempts (default: 5)\n";
    std::cout << "\n";
    
    std::cout << "📊 OUTPUT CONTROL:\n";
    std::cout << "  -v, --verbose          Enable detailed diagnostic logging\n";
    std::cout << "  -q, --quiet            Suppress non-essential output\n";
    std::cout << "  --no-color             Disable ANSI color formatting\n";
    std::cout << "  --no-banner            Skip startup banner display\n";
    std::cout << "\n";
    
    std::cout << "ℹ️  INFORMATION:\n";
    std::cout << "  -h, --help             Display this comprehensive help guide\n";
    std::cout << "  --version              Show version and build information\n";
    std::cout << "\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "📖 USAGE EXAMPLES:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "  1️⃣  Basic automatic unlock:\n";
    std::cout << "     " << program << " --auto\n\n";
    
    std::cout << "  2️⃣  Using known SystemGroup GUID:\n";
    std::cout << "     " << program << " --guid 2A22A82B-C342-444D-972F-5270FB5080DF\n\n";
    
    std::cout << "  3️⃣  Test run with detailed logs:\n";
    std::cout << "     " << program << " --dry-run --verbose\n\n";
    
    std::cout << "  4️⃣  Check connected device:\n";
    std::cout << "     " << program << " --info\n\n";
    
    std::cout << "  5️⃣  View supported devices:\n";
    std::cout << "     " << program << " --list\n";
}

/// Print version information
void print_version() {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  iOS A12+ Bypass Toolkit\n";
    std::cout << "  Version " << version::string << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "  Technology Stack:\n";
    std::cout << "    • Modern C++20 Standard\n";
    std::cout << "    • Native libimobiledevice Integration\n";
    std::cout << "    • Advanced Error Handling (Result<T> Monads)\n";
    std::cout << "    • Thread-Safe Logging System\n\n";
    std::cout << "  Copyright © 2025 - Present\n";
    std::cout << "  Licensed for Educational & Research Purposes\n";
    std::cout << "  Distributed under MIT License\n\n";
    std::cout << "  ⚠️  Disclaimer: Use responsibly and ethically.\n";
    std::cout << "     This tool is intended for legitimate device recovery only.\n";
}

/// Print a simple progress bar
void print_progress(int percent, std::string_view stage) {
    if (g_quiet_mode) return;
    
    const int bar_width = 30;
    int filled = bar_width * percent / 100;
    
    std::cout << "\r[";
    
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            if (g_colors_enabled) {
                std::cout << ansi::green << "=" << ansi::reset;
            } else {
                std::cout << "=";
            }
        } else if (i == filled) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    
    std::cout << "] " << std::setw(3) << percent << "% " << stage;
    std::cout << std::string(20, ' ');  // Clear any leftover characters
    std::cout << std::flush;
    
    if (percent >= 100) {
        std::cout << "\n";
    }
}

/// Command-line argument parser result
struct ParsedArgs {
    BypassConfig config;
    bool show_help = false;
    bool show_version = false;
    bool list_devices = false;
    bool show_device_info = false;
    bool dry_run = false;
    bool no_banner = false;
    bool no_color = false;
    fs::path output_dir;
    std::string error;
};

/// Parse command-line arguments
ParsedArgs parse_args(int argc, char* argv[]) {
    ParsedArgs result;
    
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        
        // Help and version
        if (arg == "-h" || arg == "--help") {
            result.show_help = true;
            return result;
        }
        
        if (arg == "--version") {
            result.show_version = true;
            return result;
        }
        
        // Operation modes
        if (arg == "-a" || arg == "--auto") {
            result.config.auto_mode = true;
            continue;
        }
        
        if (arg == "-n" || arg == "--dry-run") {
            result.dry_run = true;
            continue;
        }
        
        if (arg == "-l" || arg == "--list") {
            result.list_devices = true;
            continue;
        }
        
        if (arg == "-i" || arg == "--info") {
            result.show_device_info = true;
            continue;
        }
        
        // Configuration
        if ((arg == "-g" || arg == "--guid") && i + 1 < argc) {
            result.config.preset_guid = Guid{argv[++i]};
            continue;
        }
        
        if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            result.output_dir = argv[++i];
            continue;
        }
        
        if ((arg == "-t" || arg == "--timeout") && i + 1 < argc) {
            try {
                int timeout = std::stoi(argv[++i]);
                result.config.reboot_timeout = chrono::seconds{timeout};
            } catch (...) {
                result.error = "Invalid timeout value";
                return result;
            }
            continue;
        }
        
        if ((arg == "-r" || arg == "--retries") && i + 1 < argc) {
            try {
                result.config.max_guid_attempts = std::stoi(argv[++i]);
            } catch (...) {
                result.error = "Invalid retries value";
                return result;
            }
            continue;
        }
        
        // Output control
        if (arg == "-v" || arg == "--verbose") {
            result.config.verbose = true;
            continue;
        }
        
        if (arg == "-q" || arg == "--quiet") {
            g_quiet_mode = true;
            continue;
        }
        
        if (arg == "--no-color") {
            result.no_color = true;
            g_colors_enabled = false;
            continue;
        }
        
        if (arg == "--no-banner") {
            result.no_banner = true;
            continue;
        }
        
        // Unknown option
        result.error = "Unknown option: " + std::string(arg);
        return result;
    }
    
    return result;
}

/// Find and validate resources directory
std::optional<fs::path> find_resources(const char* argv0) {
    std::vector<fs::path> candidates = {
        fs::current_path() / "resources",
        fs::path(argv0).parent_path() / "resources",
#ifdef BYPASS_RESOURCES_DIR
        fs::path(BYPASS_RESOURCES_DIR),
#endif
        "/usr/local/share/a12_bypass/resources",
        "/opt/homebrew/share/a12_bypass/resources"
    };
    
    for (const auto& path : candidates) {
        if (fs::exists(path / "plists")) {
            return path;
        }
    }
    
    return std::nullopt;
}

/// Show device information
void show_device_info() {
    DeviceManager manager;
    auto result = manager.detect_device();
    
    if (!result) {
        if (g_colors_enabled) {
            std::cerr << ansi::red << "No device found: " 
                      << result.error().message() << ansi::reset << "\n";
        } else {
            std::cerr << "No device found: " << result.error().message() << "\n";
        }
        return;
    }
    
    const auto& info = result.value();
    
    std::cout << "Connected Device Information:\n";
    std::cout << std::string(40, '-') << "\n";
    std::cout << "  Device Name:      " << info.device_name << "\n";
    std::cout << "  Product Type:     " << info.product_type.get() << "\n";
    std::cout << "  Product Version:  " << info.product_version << "\n";
    std::cout << "  Build Version:    " << info.build_version << "\n";
    std::cout << "  Hardware Model:   " << info.hardware_model << "\n";
    std::cout << "  Device Class:     " << info.device_class << "\n";
    std::cout << "  Serial Number:    " << info.serial_number.get() << "\n";
    std::cout << "  UDID:             " << info.udid.get() << "\n";
    std::cout << "  Activation State: ";
    
    if (info.is_activated()) {
        if (g_colors_enabled) {
            std::cout << ansi::green << info.activation_state << ansi::reset;
        } else {
            std::cout << info.activation_state;
        }
    } else {
        if (g_colors_enabled) {
            std::cout << ansi::yellow << info.activation_state << ansi::reset;
        } else {
            std::cout << info.activation_state;
        }
    }
    std::cout << "\n";
    std::cout << std::string(40, '-') << "\n";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse arguments
    auto args = parse_args(argc, argv);
    
    if (!args.error.empty()) {
        if (g_colors_enabled) {
            std::cerr << ansi::red << "Error: " << args.error << ansi::reset << "\n\n";
        } else {
            std::cerr << "Error: " << args.error << "\n\n";
        }
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (args.show_help) {
        if (!args.no_banner) {
            print_banner();
        }
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    if (args.show_version) {
        print_version();
        return EXIT_SUCCESS;
    }
    
    // Print banner
    if (!args.no_banner) {
        print_banner();
    }
    
    // Handle device info mode
    if (args.show_device_info) {
        show_device_info();
        return EXIT_SUCCESS;
    }
    
    // Find resources directory
    auto resources = find_resources(argv[0]);
    if (!resources) {
        if (!g_quiet_mode) {
            if (g_colors_enabled) {
                std::cerr << ansi::red << "Error: Resources directory not found!" 
                          << ansi::reset << "\n";
            } else {
                std::cerr << "Error: Resources directory not found!\n";
            }
            std::cerr << "Please ensure 'resources/plists/' directory exists.\n";
        }
        return EXIT_FAILURE;
    }
    args.config.resources_path = *resources;
    
    if (args.config.verbose && !g_quiet_mode) {
        LOG_DEBUG("Using resources: ", resources->string());
    }
    
    // Handle list devices mode
    if (args.list_devices) {
        PayloadGenerator generator(*resources);
        auto devices = generator.list_supported_devices();
        
        std::cout << "Supported devices (" << devices.size() << "):\n";
        std::cout << std::string(40, '-') << "\n";
        
        // Group by type
        std::vector<std::string> iphones, ipads;
        for (const auto& device : devices) {
            if (device.find("iPhone") != std::string::npos) {
                iphones.push_back(device);
            } else if (device.find("iPad") != std::string::npos) {
                ipads.push_back(device);
            }
        }
        
        if (!iphones.empty()) {
            std::cout << "\niPhones:\n";
            for (const auto& device : iphones) {
                std::cout << "  " << device << "\n";
            }
        }
        
        if (!ipads.empty()) {
            std::cout << "\niPads:\n";
            for (const auto& device : ipads) {
                std::cout << "  " << device << "\n";
            }
        }
        
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Total: " << devices.size() << " devices\n";
        return EXIT_SUCCESS;
    }
    
    // Handle dry-run mode
    if (args.dry_run) {
        if (!g_quiet_mode) {
            if (g_colors_enabled) {
                std::cout << ansi::yellow << "[DRY RUN] " << ansi::reset 
                          << "Simulating bypass (no changes will be made)\n\n";
            } else {
                std::cout << "[DRY RUN] Simulating bypass (no changes will be made)\n\n";
            }
        }
        
        // Show what would happen
        show_device_info();
        
        if (!g_quiet_mode) {
            std::cout << "\n";
            if (g_colors_enabled) {
                std::cout << ansi::cyan << "Actions that would be performed:\n" << ansi::reset;
            } else {
                std::cout << "Actions that would be performed:\n";
            }
            std::cout << "  1. Detect connected device\n";
            if (args.config.preset_guid) {
                std::cout << "  2. Use preset GUID: " << args.config.preset_guid->get() << "\n";
            } else {
                std::cout << "  2. Extract GUID from device syslog\n";
            }
            std::cout << "  3. Generate bypass payloads\n";
            std::cout << "  4. Upload payloads to device\n";
            std::cout << "  5. Perform staged reboots\n";
            std::cout << "  6. Complete activation bypass\n";
            std::cout << "\n";
            
            if (g_colors_enabled) {
                std::cout << ansi::green << "[DRY RUN] Complete - no changes made\n" 
                          << ansi::reset;
            } else {
                std::cout << "[DRY RUN] Complete - no changes made\n";
            }
        }
        return EXIT_SUCCESS;
    }
    
    // Validate configuration
    if (!args.config.validate()) {
        if (g_colors_enabled) {
            std::cerr << ansi::red << "Error: Invalid configuration\n" << ansi::reset;
        } else {
            std::cerr << "Error: Invalid configuration\n";
        }
        return EXIT_FAILURE;
    }
    
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Create and run controller
    g_controller = std::make_unique<BypassController>(args.config);
    
    // Set progress callback
    std::string last_stage;
    g_controller->set_progress_callback(
        [&last_stage](std::string_view stage, int percent, std::string_view message) {
            if (!g_quiet_mode) {
                std::string stage_str(stage);
                if (stage_str != last_stage) {
                    if (!last_stage.empty()) {
                        // Finish previous stage line
                        print_progress(100, last_stage);
                    }
                    last_stage = stage_str;
                }
                print_progress(percent, stage);
            }
            (void)message;
        }
    );
    
    // Run bypass
    if (!g_quiet_mode) {
        std::cout << "Starting bypass procedure...\n\n";
    }
    
    auto result = g_controller->run();
    
    // Clean up
    g_controller.reset();
    
    if (!result) {
        if (!g_quiet_mode) {
            std::cout << "\n";
            if (g_colors_enabled) {
                std::cerr << ansi::red << "Bypass failed: " 
                          << result.error().message() << ansi::reset << "\n";
            } else {
                std::cerr << "Bypass failed: " << result.error().message() << "\n";
            }
        }
        return EXIT_FAILURE;
    }
    
    if (!g_quiet_mode) {
        std::cout << "\n";
        if (g_colors_enabled) {
            std::cout << ansi::green << "Bypass completed successfully!" 
                      << ansi::reset << "\n";
        } else {
            std::cout << "Bypass completed successfully!\n";
        }
    }
    
    return EXIT_SUCCESS;
}
