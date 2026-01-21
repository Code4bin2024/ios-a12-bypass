/// @file raii.hpp
/// @brief RAII wrappers for C library handles (libimobiledevice, SQLite, libzip).
/// @copyright MIT License

#pragma once

#include <concepts>
#include <cstdio>
#include <memory>
#include <utility>

// Include the actual library headers for proper type definitions
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/diagnostics_relay.h>
#include <libimobiledevice/syslog_relay.h>
#include <plist/plist.h>
#include <sqlite3.h>
#include <zip.h>

namespace a12bypass {

// ============================================================================
// Generic RAII Handle Wrapper
// ============================================================================

/// Generic RAII wrapper for C-style handles with custom deleter
template<typename T, auto Deleter>
class Handle {
public:
    using pointer = T;
    
    /// Default constructor - null handle
    constexpr Handle() noexcept = default;
    
    /// Construct from raw pointer
    constexpr explicit Handle(T ptr) noexcept : ptr_(ptr) {}
    
    /// Move constructor
    constexpr Handle(Handle&& other) noexcept 
        : ptr_(std::exchange(other.ptr_, nullptr)) {}
    
    /// Move assignment
    constexpr Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }
    
    // No copying
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    
    /// Destructor - calls deleter
    ~Handle() { reset(); }
    
    /// Reset the handle, calling deleter if non-null
    void reset(T ptr = nullptr) noexcept {
        if (ptr_) {
            Deleter(ptr_);
        }
        ptr_ = ptr;
    }
    
    /// Release ownership and return raw pointer
    [[nodiscard]] T release() noexcept {
        return std::exchange(ptr_, nullptr);
    }
    
    /// Get raw pointer
    [[nodiscard]] constexpr T get() const noexcept { return ptr_; }
    
    /// Check if handle is valid
    [[nodiscard]] constexpr explicit operator bool() const noexcept { 
        return ptr_ != nullptr; 
    }
    
    /// Dereference
    [[nodiscard]] constexpr auto operator*() const noexcept 
        requires requires { *std::declval<T>(); }
    {
        return *ptr_;
    }
    
    /// Arrow operator
    [[nodiscard]] constexpr T operator->() const noexcept { return ptr_; }
    
    /// Get address of internal pointer (for C API output parameters)
    [[nodiscard]] T* addressof() noexcept { 
        reset();  // Ensure we don't leak on reassignment
        return &ptr_; 
    }
    
    /// Swap
    constexpr void swap(Handle& other) noexcept {
        std::swap(ptr_, other.ptr_);
    }
    
private:
    T ptr_ = nullptr;
};

template<typename T, auto D>
constexpr void swap(Handle<T, D>& a, Handle<T, D>& b) noexcept {
    a.swap(b);
}

// ============================================================================
// Specific Handle Types
// ============================================================================

/// RAII wrapper for idevice_t
using DeviceHandle = Handle<idevice_t, idevice_free>;

/// RAII wrapper for lockdownd_client_t
using LockdownHandle = Handle<lockdownd_client_t, lockdownd_client_free>;

/// RAII wrapper for lockdownd_service_descriptor_t
using ServiceDescriptorHandle = Handle<
    lockdownd_service_descriptor_t, 
    lockdownd_service_descriptor_free
>;

/// RAII wrapper for afc_client_t
using AfcHandle = Handle<afc_client_t, afc_client_free>;

/// RAII wrapper for diagnostics_relay_client_t
using DiagnosticsHandle = Handle<
    diagnostics_relay_client_t, 
    diagnostics_relay_client_free
>;

/// RAII wrapper for syslog_relay_client_t
using SyslogHandle = Handle<syslog_relay_client_t, syslog_relay_client_free>;

/// RAII wrapper for plist_t
using PlistHandle = Handle<plist_t, plist_free>;

// ============================================================================
// SQLite Handle
// ============================================================================

namespace detail {
    inline void sqlite_deleter(sqlite3* db) {
        if (db) sqlite3_close(db);
    }
}

/// RAII wrapper for sqlite3*
using SqliteHandle = Handle<sqlite3*, detail::sqlite_deleter>;

// ============================================================================
// libzip Handles
// ============================================================================

namespace detail {
    inline void zip_archive_deleter(zip_t* archive) {
        if (archive) zip_discard(archive);
    }
    
    inline void zip_source_deleter(zip_source_t* source) {
        if (source) zip_source_free(source);
    }
}

/// RAII wrapper for zip_t* (archive handle)
using ZipArchiveHandle = Handle<zip_t*, detail::zip_archive_deleter>;

/// RAII wrapper for zip_source_t*
using ZipSourceHandle = Handle<zip_source_t*, detail::zip_source_deleter>;

// ============================================================================
// Scope Guard
// ============================================================================

/// Generic scope guard for cleanup actions
template<typename F>
class [[nodiscard]] ScopeGuard {
public:
    explicit ScopeGuard(F&& func) noexcept
        : func_(std::forward<F>(func))
        , active_(true)
    {}
    
    ScopeGuard(ScopeGuard&& other) noexcept
        : func_(std::move(other.func_))
        , active_(std::exchange(other.active_, false))
    {}
    
    ~ScopeGuard() {
        if (active_) {
            func_();
        }
    }
    
    /// Dismiss the guard - cleanup won't be called
    void dismiss() noexcept { active_ = false; }
    
    // No copying
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;
    
private:
    F func_;
    bool active_;
};

/// Create a scope guard with automatic type deduction
template<typename F>
[[nodiscard]] ScopeGuard<F> make_scope_guard(F&& func) {
    return ScopeGuard<F>(std::forward<F>(func));
}

/// Macro for anonymous scope guards
#define BYPASS_SCOPE_EXIT(code) \
    auto BYPASS_CONCAT_(_scope_guard_, __LINE__) = \
        ::a12bypass::make_scope_guard([&]() { code; })

#define BYPASS_CONCAT_(a, b) BYPASS_CONCAT2_(a, b)
#define BYPASS_CONCAT2_(a, b) a##b

// ============================================================================
// File Handle RAII
// ============================================================================

/// RAII wrapper for FILE*
class FileHandle {
public:
    FileHandle() = default;
    
    explicit FileHandle(FILE* fp) noexcept : fp_(fp) {}
    
    explicit FileHandle(const char* path, const char* mode) noexcept
        : fp_(std::fopen(path, mode))
    {}
    
    ~FileHandle() {
        if (fp_) std::fclose(fp_);
    }
    
    FileHandle(FileHandle&& other) noexcept 
        : fp_(std::exchange(other.fp_, nullptr)) {}
    
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (fp_) std::fclose(fp_);
            fp_ = std::exchange(other.fp_, nullptr);
        }
        return *this;
    }
    
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    
    [[nodiscard]] FILE* get() const noexcept { return fp_; }
    [[nodiscard]] explicit operator bool() const noexcept { return fp_ != nullptr; }
    
    FILE* release() noexcept { return std::exchange(fp_, nullptr); }
    
private:
    FILE* fp_ = nullptr;
};

} // namespace a12bypass
