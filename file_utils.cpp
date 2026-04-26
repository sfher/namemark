// file_utils.cpp
#include "file_utils.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace FileUtils {

    std::vector<std::string> scan_directory(const std::string& dir_path, const std::string& extension) {
        std::vector<std::string> files;
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            std::cerr << "[FileUtils] directory not found: " << dir_path << std::endl;
            return files;
        }
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

} // namespace FileUtils