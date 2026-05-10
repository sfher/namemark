// monster_preset.cpp
#include "monster_preset.h"
#include "json.hpp"
#include "customio.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool MonsterPresetManager::load_from_json(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open monster preset file: " << filepath << std::endl;
        return false;
    }

    try {
        json data = json::parse(file);
        for (const auto& item : data["monsters"]) {
            MonsterPreset preset;
            preset.id = item.value("id", "");
            if (preset.id.empty()) continue;
            preset.display_name = Utf8ToAnsi(item.value("name", preset.id));

            if (item.contains("attributes")) {
                for (auto& [key, val] : item["attributes"].items()) {
                    preset.attributes[key] = val.get<int>();
                }
            }

            if (item.contains("actions")) {
                for (const auto& action : item["actions"]) {
                    preset.actions.push_back(action.get<std::string>());
                }
            }

            presets_[preset.id] = std::move(preset);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Monster preset JSON parse error: " << e.what() << std::endl;
        return false;
    }

    std::cout << "Loaded " << presets_.size() << " monster presets." << std::endl;
    return true;
}

bool MonsterPresetManager::load_from_package(const std::string& package_path) {
    presets_.clear();
    std::string filepath = package_path + "/monsters.json";
    return load_from_json(filepath);
}

const MonsterPreset* MonsterPresetManager::get_preset(const std::string& id) const {
    auto it = presets_.find(id);
    return it != presets_.end() ? &it->second : nullptr;
}

bool MonsterPresetManager::has_preset(const std::string& id) const {
    return presets_.find(id) != presets_.end();
}