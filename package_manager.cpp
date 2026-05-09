// package_manager.cpp
#include "package_manager.h"
#include <filesystem>
#include <iostream>

std::vector<PackageInfo> PackageManager::scan() {
    std::vector<PackageInfo> result;
    std::error_code ec;

    if (!std::filesystem::exists("packages", ec) || !std::filesystem::is_directory("packages", ec)) {
        std::cerr << "[PackageManager] 未找到 packages/ 目录。\n";
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator("packages", ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        std::string dir_name = entry.path().filename().string();
        bool has_monsters = std::filesystem::exists(entry.path() / "monsters.json", ec);
        bool has_levels   = std::filesystem::exists(entry.path() / "levels.json", ec);

        if (has_monsters || has_levels) {
            PackageInfo info;
            info.name    = dir_name;
            info.path    = entry.path().string();
            info.display = dir_name;
            result.push_back(info);
        }
    }

    return result;
}
