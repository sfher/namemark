// weapon_loader.cpp
#include "weapon_loader.h"
#include "file_utils.h"
#include "customio.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::vector<WeaponData> WeaponLoader::load_from_file(const std::string& filepath) {
    std::vector<WeaponData> result;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[WeaponLoader] cannot open: " << filepath << std::endl;
        return result;
    }
    json data;
    file >> data;
    if (!data.contains("weapons") || !data["weapons"].is_array()) return result;
    for (const auto& item : data["weapons"]) {
        WeaponData w;
        w.id = item.value("id", "");
        w.name = Utf8ToAnsi(item.value("name", w.id));
        w.description = Utf8ToAnsi(item.value("description", ""));
        w.rarity = item.value("rarity", "R");
        w.price = item.value("price", 0);
        if (item.contains("attributes")) {
            for (auto& [key, val] : item["attributes"].items()) {
                w.attributes[key] = val.get<int>();
            }
        }
        w.skill_grant = item.value("skill_grant", "");
        w.skill_override = item.value("skill_override", "");
        result.push_back(w);
    }
    return result;
}

std::vector<WeaponData> WeaponLoader::load_from_directory(const std::string& dir_path) {
    std::vector<WeaponData> result;
    auto files = FileUtils::scan_directory(dir_path, ".json");
    for (const auto& file : files) {
        auto weapons = load_from_file(file);
        result.insert(result.end(), weapons.begin(), weapons.end());
    }
    return result;
}