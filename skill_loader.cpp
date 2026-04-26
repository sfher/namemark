// skill_loader.cpp
#include "skill_loader.h"
#include "file_utils.h"
#include "customio.h"          // 新增：提供 Utf8ToAnsi
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::vector<SkillData> SkillLoader::load_from_file(const std::string& filepath) {
    std::vector<SkillData> result;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SkillLoader] cannot open: " << filepath << std::endl;
        return result;
    }
    json data;
    file >> data;
    if (!data.contains("skills") || !data["skills"].is_array()) return result;
    for (const auto& item : data["skills"]) {
        result.push_back(parse_skill(item));
    }
    return result;
}

std::vector<SkillData> SkillLoader::load_from_directory(const std::string& dir_path) {
    std::vector<SkillData> result;
    auto files = FileUtils::scan_directory(dir_path, ".json");
    for (const auto& file : files) {
        auto skills = load_from_file(file);
        result.insert(result.end(), skills.begin(), skills.end());
    }
    return result;
}

SkillData SkillLoader::parse_skill(const json& j) {
    SkillData skill;
    skill.id = j.value("id", "");
    // 关键修改：对中文文本应用 Utf8ToAnsi 转码
    skill.display_name = Utf8ToAnsi(j.value("display_name", skill.id));
    skill.description  = Utf8ToAnsi(j.value("description", ""));
    skill.acquire_chance = j.value("acquire_chance", 100);
    skill.weight = j.value("weight", 5);
    skill.base_probability = j.value("base_probability", 100);

    if (j.contains("consume")) {
        for (auto it = j["consume"].begin(); it != j["consume"].end(); ++it) {
            skill.consume[it.key()] = it.value().get<int>();
        }
    }

    skill.target_type = parse_target_type(j.value("target_type", "SINGLE_ENEMY_LOWEST_HP"));
    skill.type = parse_executor_type(j.value("executor_type", "SINGLE_DAMAGE"));

    skill.hit_rate = j.value("hit_rate", 100);
    skill.dmg_formula = parse_dmg_formula(j.value("dmg_formula", "ATK"));
    skill.dmg_coeff = j.value("dmg_coeff", 1.0f);
    skill.use_def = j.value("use_def", true);
    skill.split_damage = j.value("split_damage", false);

    if (j.contains("on_hit_buffs")) skill.on_hit_buffs = parse_buffs(j["on_hit_buffs"]);
    if (j.contains("self_buffs")) skill.self_buffs = parse_buffs(j["self_buffs"]);

    if (j.contains("attr_modifiers")) {
        for (auto it = j["attr_modifiers"].begin(); it != j["attr_modifiers"].end(); ++it) {
            skill.attr_modifiers[it.key()] = it.value().get<int>();
        }
    }

    skill.heal_func = nullptr;
    skill.custom_factory = nullptr;

    skill.random_available = j.value("random_available", true);

    return skill;
}

TargetType SkillLoader::parse_target_type(const std::string& s) {
    if (s == "SINGLE_ENEMY_RANDOM")       return TargetType::SINGLE_ENEMY_RANDOM;
    if (s == "SINGLE_ENEMY_HIGHEST_ATK")  return TargetType::SINGLE_ENEMY_HIGHEST_ATK;
    if (s == "SINGLE_ALLY_LOWEST_HP")    return TargetType::SINGLE_ALLY_LOWEST_HP;
    if (s == "SINGLE_ALLY_SELF")         return TargetType::SINGLE_ALLY_SELF;
    if (s == "ALL_ENEMIES")              return TargetType::ALL_ENEMIES;
    if (s == "ALL_ALLIES")               return TargetType::ALL_ALLIES;
    if (s == "NONE")                     return TargetType::NONE;
    return TargetType::SINGLE_ENEMY_LOWEST_HP;
}

SkillData::Type SkillLoader::parse_executor_type(const std::string& s) {
    if (s == "AOE_DAMAGE")   return SkillData::Type::AOE_DAMAGE;
    if (s == "HEAL")         return SkillData::Type::HEAL;
    if (s == "SELF_BUFF")    return SkillData::Type::SELF_BUFF;
    if (s == "CUSTOM")       return SkillData::Type::CUSTOM;
    return SkillData::Type::SINGLE_DAMAGE;
}

DamageFormula SkillLoader::parse_dmg_formula(const std::string& s) {
    if (s == "MATK")              return DamageFormula::MATK;
    if (s == "ATK_MATK_SUM")      return DamageFormula::ATK_MATK_SUM;
    if (s == "ATK_MATK_COMBINED") return DamageFormula::ATK_MATK_COMBINED;
    if (s == "ATK_POWER")         return DamageFormula::ATK_POWER;
    if (s == "MATK_POWER")        return DamageFormula::MATK_POWER;
    if (s == "CUSTOM")            return DamageFormula::CUSTOM;
    return DamageFormula::ATK;
}

std::vector<BuffApplication> SkillLoader::parse_buffs(const json& j) {
    std::vector<BuffApplication> result;
    if (!j.is_array()) return result;
    for (const auto& item : j) {
        BuffApplication buff;
        buff.buff_name = item.value("buff_name", "");
        buff.effect    = item.value("effect", 0);
        buff.duration  = item.value("duration", 1);
        result.push_back(buff);
    }
    return result;
}