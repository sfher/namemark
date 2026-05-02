// package_manager.cpp
#include "package_manager.h"
#define NOMINMAX
#include <windows.h>
#include <iostream>

std::vector<PackageInfo> PackageManager::scan() {
    std::vector<PackageInfo> result;
    std::string search_path = "packages\\*";

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "[PackageManager] 未找到 packages/ 目录。\n";
        return result;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::string dir_name = find_data.cFileName;
            std::string full_path = "packages/" + dir_name;

            // 检查该目录下是否存在 monsters.json 或 levels.json
            std::string monster_file = full_path + "/monsters.json";
            std::string level_file = full_path + "/levels.json";

            bool has_monsters = (GetFileAttributesA(monster_file.c_str()) != INVALID_FILE_ATTRIBUTES);
            bool has_levels = (GetFileAttributesA(level_file.c_str()) != INVALID_FILE_ATTRIBUTES);

            if (has_monsters || has_levels) {
                PackageInfo info;
                info.name = dir_name;
                info.path = full_path;
                info.display = dir_name; // 后续可读取 package.json 获取显示名
                result.push_back(info);
            }
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
    return result;
}