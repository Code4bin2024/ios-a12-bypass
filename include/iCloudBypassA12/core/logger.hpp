/// @file logger.hpp
/// @brief Modern thread-safe logging facility with source location support.
/// @copyright MIT License

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <ostream>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

namespace a12bypass {

// ============================================================================
// ANSI Color Codes
// ============================================================================

namespace ansi {

inline constexpr std::string_view reset   = "\033[0m";
inline constexpr std::string_view bold    = "\033[1m";
inline constexpr std::string_view dim     = "\033[2m";
inline constexpr std::string_view italic  = "\033[3m";

inline constexpr std::string_view black   = "\033[30m";
inline constexpr std::string_view red     = "\033[31m";
inline constexpr std::string_view green   = "\033[32m";
inline constexpr std::string_view yellow  = "\033[33m";
inline constexpr std::string_view blue    = "\033[34m";
inline constexpr std::string_view magenta = "\033[35m";
inline constexpr std::string_view cyan    = "\033[36m";
inline constexpr std::string_view white   = "\033[37m";

inline constexpr std::string_view bg_red    = "\033[41m";
inline constexpr std::string_view bg_green  = "\033[42m";
inline constexpr std::string_view bg_yellow = "\033[43m";
inline constexpr std::string_view bg_blue   = "\033[44m";

} // namespace ansi

// ============================================================================
// Log Levels
// ============================================================================

enum class LogLevel : std::uint8_t {
    Trace   = 0,
    Debug   = 1,
    Info    = 2,
    Success = 3,
    Warning = 4,
    Error   = 5,
    Fatal   = 6,
    Step    = 7,   // Special level for major steps
    None    = 255  // Disable all logging
};

/// Convert log level to string
[[nodiscard]] constexpr std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Success: return "OK";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        case LogLevel::Step:    return "STEP";
        case LogLevel::None:    return "NONE";
    }
    return "???";
}

/// Get ANSI color for log level
[[nodiscard]] constexpr std::string_view color_for(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:   return ansi::dim;
        case LogLevel::Debug:   return ansi::cyan;
        case LogLevel::Info:    return ansi::green;
        case LogLevel::Success: return ansi::green;
        case LogLevel::Warning: return ansi::yellow;
        case LogLevel::Error:   return ansi::red;
        case LogLevel::Fatal:   return ansi::bg_red;
        case LogLevel::Step:    return ansi::blue;
        default:                return ansi::reset;
    }
}

// ============================================================================
// Logger Configuration
// ============================================================================

struct LoggerConfig {
    LogLevel min_level = LogLevel::Info;
    bool show_timestamp = false;
    bool show_source_location = false;
    bool use_colors = true;
    std::ostream* output = nullptr;  // nullptr = stdout/stderr
};

// ============================================================================
// Logger Class
// ============================================================================

/// Thread-safe logging facility
class Logger {
public:
    /// Get the singleton instance
    [[nodiscard]] static Logger& instance() noexcept;
    
    /// Configure the logger
    void configure(const LoggerConfig& config) noexcept;
    
    /// Set minimum log level
    void set_level(LogLevel level) noexcept {
        min_level_.store(level, std::memory_order_relaxed);
    }
    
    /// Get current log level
    [[nodiscard]] LogLevel level() const noexcept {
        return min_level_.load(std::memory_order_relaxed);
    }
    
    /// Enable/disable verbose mode (shows Debug level)
    void set_verbose(bool verbose) noexcept {
        set_level(verbose ? LogLevel::Debug : LogLevel::Info);
    }
    
    /// Check if verbose mode is enabled
    [[nodiscard]] bool is_verbose() const noexcept {
        return min_level_.load(std::memory_order_relaxed) <= LogLevel::Debug;
    }
    
    /// Enable/disable colors
    void set_colors(bool enabled) noexcept {
        use_colors_.store(enabled, std::memory_order_relaxed);
    }
    
    // ========================================================================
    // Logging Methods
    // ========================================================================
    
    /// Log a message with variadic arguments
    template<typename... Args>
    void log(
        LogLevel level,
        std::source_location loc,
        Args&&... args
    ) {
        if (!should_log(level)) return;
        
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        write_log(level, loc, oss.str());
    }
    
    /// Convenience methods
    template<typename... Args>
    void trace(Args&&... args) {
        log(LogLevel::Trace, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void debug(Args&&... args) {
        log(LogLevel::Debug, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(Args&&... args) {
        log(LogLevel::Info, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void success(Args&&... args) {
        log(LogLevel::Success, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(Args&&... args) {
        log(LogLevel::Warning, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(Args&&... args) {
        log(LogLevel::Error, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void fatal(Args&&... args) {
        log(LogLevel::Fatal, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void step(Args&&... args) {
        log(LogLevel::Step, std::source_location::current(), std::forward<Args>(args)...);
    }
    
    /// Log a detailed sub-item
    template<typename... Args>
    void detail(Args&&... args) {
        if (!should_log(LogLevel::Info)) return;
        
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        write_detail(oss.str());
    }
    
private:
    Logger() = default;
    ~Logger() = default;
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    [[nodiscard]] bool should_log(LogLevel level) const noexcept {
        return level >= min_level_.load(std::memory_order_relaxed);
    }
    
    void write_log(LogLevel level, std::source_location loc, std::string_view message);
    void write_detail(std::string_view message);
    
    std::mutex mutex_;
    std::atomic<LogLevel> min_level_{LogLevel::Info};
    std::atomic<bool> use_colors_{true};
    std::atomic<bool> show_timestamp_{false};
    std::atomic<bool> show_location_{false};
    std::ostream* output_ = nullptr;
};

// ============================================================================
// Convenience Macros
// ============================================================================

#define LOG_TRACE(...) ::a12bypass::Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::a12bypass::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)  ::a12bypass::Logger::instance().info(__VA_ARGS__)
#define LOG_SUCCESS(...) ::a12bypass::Logger::instance().success(__VA_ARGS__)
#define LOG_WARN(...)  ::a12bypass::Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) ::a12bypass::Logger::instance().error(__VA_ARGS__)
#define LOG_FATAL(...) ::a12bypass::Logger::instance().fatal(__VA_ARGS__)
#define LOG_STEP(...)  ::a12bypass::Logger::instance().step(__VA_ARGS__)
#define LOG_DETAIL(...) ::a12bypass::Logger::instance().detail(__VA_ARGS__)

} // namespace a12bypass
