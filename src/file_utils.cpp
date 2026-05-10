// file_utils.cpp
#include "file_utils.h"
#include <filesystem>
#include <algorithm>

namespace FileUtils {

    std::vector<std::string> scan_directory(const std::string& dir_path, const std::string& extension) {
        std::vector<std::string> files;
        std::error_code ec;

        if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec)) {
            return files;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir_path, ec)) {
            if (ec) break;
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                files.push_back(entry.path().string());
            }
        }

        std::sort(files.begin(), files.end());
        return files;
    }

    std::vector<std::string> scan_directory_recursive(const std::string& dir_path, const std::string& extension) {
        std::vector<std::string> files;
        std::error_code ec;

        if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec)) {
            return files;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path, ec)) {
            if (ec) break;
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                files.push_back(entry.path().string());
            }
        }

        std::sort(files.begin(), files.end());
        return files;
    }

    bool file_exists(const std::string& path) {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
    }

    bool dir_exists(const std::string& path) {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
    }

} // namespace FileUtils
