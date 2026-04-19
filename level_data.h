// level_data.h
#pragma once

#include "monster_preset.h"
#include <string>
#include <vector>
#include <unordered_map>

struct EnemySpawn {
    std::string preset_id;                              // 预设ID（优先）
    std::string name;                                   // 直接名称（向后兼容）
    std::unordered_map<std::string, int> attributes;    // 直接属性或覆盖属性
};

struct LevelData {
    std::string id;
    std::string name;
    std::string description;
    int max_allies = 3;
    std::vector<EnemySpawn> enemies;
    bool unlocked = false;   // 是否已解锁
    bool completed = false;  // 是否已完成
};

class LevelManager {
public:
    bool load_from_json(const std::string& filepath);

    size_t get_level_count() const { return levels_.size(); }
    LevelData* get_level(int index);
    const LevelData* get_level(int index) const;

    bool is_unlocked(int index) const;
    bool is_completed(int index) const;

    void mark_completed(int index);
    void unlock_next(int current_index);  // 解锁下一关

    void set_preset_manager(MonsterPresetManager* manager) { preset_manager_ = manager; }

private:
    MonsterPresetManager* preset_manager_ = nullptr;

    std::vector<LevelData> levels_;
};