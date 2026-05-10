// weapon_loader.h
#pragma once
#include "weapon_data.h"
#include <vector>
#include <string>

class WeaponLoader {
public:
    static std::vector<WeaponData> load_from_file(const std::string& filepath);
    static std::vector<WeaponData> load_from_directory(const std::string& dir_path);
};