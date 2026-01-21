/// @file error.cpp
/// @brief Error handling and reporting implementation
/// @copyright MIT License - Educational & Research Use Only
/// @warning This software is for educational purposes. Use responsibly.

#include "iCloudBypassA12/core/error.hpp"
#include <sstream>

namespace a12bypass {

std::string Error::to_string() const {
    if (code_ == ErrorCode::Success) {
        return "Success";
    }
    
    std::ostringstream oss;
    oss << "[" << a12bypass::to_string(category()) << "] ";
    
    if (!message_.empty()) {
        oss << message_;
    } else {
        oss << "Error code " << static_cast<int>(code_);
    }
    
    return oss.str();
}

std::string Error::to_diagnostic_string() const {
    if (code_ == ErrorCode::Success) {
        return "Success";
    }
    
    std::ostringstream oss;
    oss << "[" << a12bypass::to_string(category()) << "] ";
    
    if (!message_.empty()) {
        oss << message_;
    } else {
        oss << "Error code " << static_cast<int>(code_);
    }
    
    oss << "\n  at " << location_.file_name() 
        << ":" << location_.line()
        << " in " << location_.function_name();
    
    return oss.str();
}

} // namespace a12bypass
