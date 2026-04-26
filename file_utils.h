// file_utils.h
#pragma once
#include <string>
#include <vector>

namespace FileUtils {
    std::vector<std::string> scan_directory(const std::string& dir_path, const std::string& extension = ".json");
}