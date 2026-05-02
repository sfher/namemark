// file_utils.cpp
#include "file_utils.h"
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>

namespace FileUtils {

    std::vector<std::string> scan_directory(const std::string& dir_path, const std::string& extension) {
        std::vector<std::string> files;

        // 构造搜索路径，例如 "config/skills/*.json"
        std::string search_path = dir_path + "\\*" + extension;

        WIN32_FIND_DATAA find_data;
        HANDLE hFind = FindFirstFileA(search_path.c_str(), &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            // 目录不存在或没有匹配文件，静默返回
            return files;
        }

        do {
            // 跳过 "." 和 ".."
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
                continue;
            }
            // 构造完整路径
            std::string full_path = dir_path + "\\" + find_data.cFileName;
            files.push_back(full_path);
        } while (FindNextFileA(hFind, &find_data));

        FindClose(hFind);
        return files;
    }

    // 如果需要递归扫描子目录，可以在这里加一个递归版本，但目前的用法只需要一层
    // 为了兼容，这里保持原来的函数签名，但只扫描一级目录
    std::vector<std::string> scan_directory_recursive(const std::string& dir_path, const std::string& extension) {
        // 简单起见，暂时只返回一级文件，你的现有代码也只用到一级扫描
        return scan_directory(dir_path, extension);
    }

    bool file_exists(const std::string& path) {
        DWORD attrib = GetFileAttributesA(path.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    bool dir_exists(const std::string& path) {
        DWORD attrib = GetFileAttributesA(path.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
    }

} // namespace FileUtils