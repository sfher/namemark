// weapon_data.h
#pragma once
#include <string>
#include <unordered_map>

struct WeaponData {
    std::string id;
    std::string name;
    std::string description;
    std::string rarity;
    int price = 0;
    std::unordered_map<std::string, int> attributes;
    std::string skill_grant;
    std::string skill_override;
    bool equipped = false;
};