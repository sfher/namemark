// level_data.h
#pragma once
#include "monster_preset.h" // 如果你使用预设管理器
#include <string>
#include <vector>
#include <unordered_map>

struct EnemySpawn {
    std::string preset_id;
    std::string name;
    std::unordered_map<std::string, int> attributes;
};

struct LevelData {
    std::string id;
    std::string name;
    std::string description;
    int max_allies = 3;
    std::vector<EnemySpawn> enemies;
    bool unlocked = false;
    bool completed = false;
};

class LevelManager {
public:
    bool load_from_json(const std::string& filepath);
    bool load_from_directory(const std::string& dir_path);   // 新增方法

    size_t get_level_count() const { return levels_.size(); }
    LevelData* get_level(int index);
    const LevelData* get_level(int index) const;

    bool is_unlocked(int index) const;
    bool is_completed(int index) const;

    void mark_completed(int index);
    void unlock_next(int current_index);

    void set_preset_manager(MonsterPresetManager* manager) { preset_manager_ = manager; }

private:
    std::vector<LevelData> levels_;
    MonsterPresetManager* preset_manager_ = nullptr;
};