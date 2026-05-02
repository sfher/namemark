// file_utils.h
#pragma once
#include <string>
#include <vector>

namespace FileUtils {
    std::vector<std::string> scan_directory(const std::string& dir_path, const std::string& extension = ".json");
    std::vector<std::string> scan_directory_recursive(const std::string& dir_path, const std::string& extension = ".json");
    bool file_exists(const std::string& path);
    bool dir_exists(const std::string& path);
}