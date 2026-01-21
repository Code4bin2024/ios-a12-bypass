/// @file plist_utils.cpp
/// @brief Plist utilities implementation.
/// @copyright MIT License

#include "iCloudBypassA12/utils/plist_utils.hpp"
#include "iCloudBypassA12/core/logger.hpp"
#include <fstream>
#include <cstring>

namespace a12bypass {

std::optional<std::unordered_map<std::string, std::string>>
PlistUtils::parse_file(const fs::path& path) {
    auto data = read_file(path);
    if (!data) {
        return std::nullopt;
    }
    return parse_data(*data);
}

std::optional<std::unordered_map<std::string, std::string>>
PlistUtils::parse_data(ByteSpan data) {
    plist_t root = nullptr;
    plist_format_t format;
    plist_err_t err = plist_from_memory(
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::uint32_t>(data.size()),
        &root,
        &format
    );
    
    if (err != PLIST_ERR_SUCCESS || !root) {
        LOG_ERROR("Failed to parse plist data");
        return std::nullopt;
    }
    
    PlistHandle plist_guard{root};
    
    std::unordered_map<std::string, std::string> result;
    extract_dict_values(root, result);
    
    return result;
}

std::optional<std::unordered_map<std::string, std::string>>
PlistUtils::parse_xml(std::string_view xml) {
    plist_t root = nullptr;
    plist_err_t err = plist_from_xml(
        xml.data(), 
        static_cast<std::uint32_t>(xml.size()), 
        &root
    );
    
    if (err != PLIST_ERR_SUCCESS || !root) {
        LOG_ERROR("Failed to parse XML plist");
        return std::nullopt;
    }
    
    PlistHandle plist_guard{root};
    
    std::unordered_map<std::string, std::string> result;
    extract_dict_values(root, result);
    
    return result;
}

std::optional<ByteBuffer> PlistUtils::read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_ERROR("Failed to open file: ", path.string());
        return std::nullopt;
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    ByteBuffer data(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        LOG_ERROR("Failed to read file: ", path.string());
        return std::nullopt;
    }
    
    return data;
}

bool PlistUtils::write_file(const fs::path& path, ByteSpan data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        LOG_ERROR("Failed to create file: ", path.string());
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), 
               static_cast<std::streamsize>(data.size()));
    return file.good();
}

std::optional<ByteBuffer> PlistUtils::xml_to_binary(std::string_view xml) {
    plist_t root = nullptr;
    plist_err_t err = plist_from_xml(
        xml.data(), 
        static_cast<std::uint32_t>(xml.size()), 
        &root
    );
    
    if (err != PLIST_ERR_SUCCESS || !root) {
        return std::nullopt;
    }
    
    PlistHandle plist_guard{root};
    
    char* bin = nullptr;
    std::uint32_t length = 0;
    plist_to_bin(root, &bin, &length);
    
    if (!bin) {
        return std::nullopt;
    }
    
    ByteBuffer result(bin, bin + length);
    free(bin);
    return result;
}

std::optional<std::string> PlistUtils::binary_to_xml(ByteSpan data) {
    plist_t root = nullptr;
    plist_err_t err = plist_from_bin(
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::uint32_t>(data.size()),
        &root
    );
    
    if (err != PLIST_ERR_SUCCESS || !root) {
        return std::nullopt;
    }
    
    PlistHandle plist_guard{root};
    
    char* xml = nullptr;
    std::uint32_t length = 0;
    plist_to_xml(root, &xml, &length);
    
    if (!xml) {
        return std::nullopt;
    }
    
    std::string result(xml, length);
    free(xml);
    return result;
}

void PlistUtils::extract_dict_values(
    plist_t node,
    std::unordered_map<std::string, std::string>& result
) {
    if (plist_get_node_type(node) != PLIST_DICT) {
        return;
    }
    
    plist_dict_iter iter = nullptr;
    plist_dict_new_iter(node, &iter);
    
    char* key = nullptr;
    plist_t val = nullptr;
    
    while (true) {
        plist_dict_next_item(node, iter, &key, &val);
        if (!key) break;
        
        if (plist_get_node_type(val) == PLIST_STRING) {
            char* str_val = nullptr;
            plist_get_string_val(val, &str_val);
            if (str_val) {
                result[key] = str_val;
                free(str_val);
            }
        }
        free(key);
        key = nullptr;
    }
    
    free(iter);
}

} // namespace a12bypass
