/// @file logger.cpp
/// @brief Thread-safe logging system implementation
/// @copyright MIT License - Educational & Research Use Only
/// @warning This software is for educational purposes. Use responsibly.

#include "iCloudBypassA12/core/logger.hpp"
#include <iostream>
#include <iomanip>

namespace a12bypass {

Logger& Logger::instance() noexcept {
    static Logger logger;
    return logger;
}

void Logger::configure(const LoggerConfig& config) noexcept {
    std::lock_guard lock(mutex_);
    
    min_level_.store(config.min_level, std::memory_order_relaxed);
    use_colors_.store(config.use_colors, std::memory_order_relaxed);
    show_timestamp_.store(config.show_timestamp, std::memory_order_relaxed);
    show_location_.store(config.show_source_location, std::memory_order_relaxed);
    output_ = config.output;
}

void Logger::write_log(
    LogLevel level, 
    std::source_location loc, 
    std::string_view message
) {
    std::lock_guard lock(mutex_);
    
    std::ostream& out = (level >= LogLevel::Error) 
        ? (output_ ? *output_ : std::cerr)
        : (output_ ? *output_ : std::cout);
    
    const bool colors = use_colors_.load(std::memory_order_relaxed);
    
    // Handle Step level specially
    if (level == LogLevel::Step) {
        if (colors) {
            out << "\n" << ansi::cyan;
        } else {
            out << "\n";
        }
        out << std::string(50, '=');
        if (colors) out << ansi::reset;
        out << "\n";
        
        if (colors) {
            out << ansi::blue << ansi::bold << "> " << ansi::reset 
                << ansi::bold << message << ansi::reset;
        } else {
            out << "> " << message;
        }
        out << "\n";
        
        if (colors) out << ansi::cyan;
        out << std::string(50, '=');
        if (colors) out << ansi::reset;
        out << "\n";
        
        out.flush();
        return;
    }
    
    // Regular log message
    if (show_timestamp_.load(std::memory_order_relaxed)) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count() % 1000;
        
        out << std::put_time(std::localtime(&time), "%H:%M:%S");
        out << "." << std::setfill('0') << std::setw(3) << ms << " ";
    }
    
    // Level prefix
    if (colors) {
        out << color_for(level);
        if (level == LogLevel::Success) {
            out << ansi::bold;
        }
    }
    
    switch (level) {
        case LogLevel::Trace:   out << "[TRACE] "; break;
        case LogLevel::Debug:   out << "[DEBUG] "; break;
        case LogLevel::Info:    out << "[+] "; break;
        case LogLevel::Success: out << "[OK] "; break;
        case LogLevel::Warning: out << "[!] "; break;
        case LogLevel::Error:   out << "[X] "; break;
        case LogLevel::Fatal:   out << "[FATAL] "; break;
        default: break;
    }
    
    if (colors) {
        out << ansi::reset;
    }
    
    // Message
    out << message;
    
    // Source location (if enabled and debug level)
    if (show_location_.load(std::memory_order_relaxed) && 
        level <= LogLevel::Debug) {
        if (colors) out << ansi::dim;
        out << " [" << loc.file_name() << ":" << loc.line() << "]";
        if (colors) out << ansi::reset;
    }
    
    out << "\n";
    out.flush();
}

void Logger::write_detail(std::string_view message) {
    std::lock_guard lock(mutex_);
    
    std::ostream& out = output_ ? *output_ : std::cout;
    const bool colors = use_colors_.load(std::memory_order_relaxed);
    
    if (colors) {
        out << ansi::cyan << "  -> " << ansi::reset;
    } else {
        out << "  -> ";
    }
    
    out << message << "\n";
    out.flush();
}

} // namespace a12bypass
