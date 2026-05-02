// level_data.cpp
#include "level_data.h"
#include "json.hpp"
#include "file_utils.h"
#include "customio.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool LevelManager::load_from_json(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level file: " << filepath << std::endl;
        return false;
    }

    try {
        json data = json::parse(file);
        for (const auto& level_item : data["levels"]) {   // 使用 level_item 避免与 LevelData 混淆
            LevelData lv;                           
            lv.id = level_item.value("id", "");
            lv.name = Utf8ToAnsi(level_item.value("name", ""));
            lv.description = Utf8ToAnsi(level_item.value("description", ""));
            lv.max_allies = level_item.value("max_allies", 3);
            lv.unlocked = level_item.value("unlocked", (levels_.size() == 0));
            lv.completed = level_item.value("completed", false);
            lv.reward_gold = level_item.value("reward_gold", 300);

            for (const auto& enemy_item : level_item["enemies"]) {   // 使用 enemy_item
                EnemySpawn spawn;
                if (enemy_item.contains("preset_id")) {
                    spawn.preset_id = enemy_item["preset_id"].get<std::string>();
                }
                if (enemy_item.contains("name")) {
                    spawn.name = Utf8ToAnsi(enemy_item["name"].get<std::string>());
                }
                if (enemy_item.contains("attributes")) {
                    for (auto& [key, val] : enemy_item["attributes"].items()) {
                        spawn.attributes[key] = val.get<int>();
                    }
                }
                lv.enemies.push_back(spawn);
            }
            levels_.push_back(lv);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

LevelData* LevelManager::get_level(int index) {
    if (index >= 0 && index < (int)levels_.size()) return &levels_[index];
    return nullptr;
}

bool LevelManager::is_unlocked(int index) const {
    if (index >= 0 && index < (int)levels_.size())
        return levels_[index].unlocked;
    return false;
}

bool LevelManager::is_completed(int index) const {
    if (index >= 0 && index < (int)levels_.size())
        return levels_[index].completed;
    return false;
}

void LevelManager::mark_completed(int index) {
    if (index >= 0 && index < (int)levels_.size()) {
        levels_[index].completed = true;
    }
}

void LevelManager::unlock_next(int current_index) {
    int next = current_index + 1;
    if (next < (int)levels_.size()) {
        levels_[next].unlocked = true;
    }
}

bool LevelManager::load_from_directory(const std::string& dir_path) {
    auto files = FileUtils::scan_directory(dir_path, ".json");
    bool any_loaded = false;
    for (const auto& file : files) {
        std::cout << "[LevelManager] loading: " << file << std::endl;
        if (load_from_json(file)) {
            any_loaded = true;
        }
    }
    return any_loaded;
}

bool LevelManager::load_from_package(const std::string& package_path) {
    levels_.clear();  // 清空旧数据
    std::string filepath = package_path + "/levels.json";
    return load_from_json(filepath);
}