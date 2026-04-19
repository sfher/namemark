// monster_preset.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct MonsterPreset {
    std::string id;
    std::string display_name;
    std::unordered_map<std::string, int> attributes;
    std::vector<std::string> actions;
};

class MonsterPresetManager {
public:
    bool load_from_json(const std::string& filepath);
    const MonsterPreset* get_preset(const std::string& id) const;
    bool has_preset(const std::string& id) const;

private:
    std::unordered_map<std::string, MonsterPreset> presets_;
};