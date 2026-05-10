// skill_loader.h
#pragma once

#include "skill_data.h"
#include "json.hpp"
#include <string>
#include <vector>

class SkillLoader {
public:
    // 从单个 JSON 文件加载技能列表
    static std::vector<SkillData> load_from_file(const std::string& filepath);
    // 从目录递归加载所有 JSON 文件中的技能
    static std::vector<SkillData> load_from_directory(const std::string& dir_path);

private:
    static SkillData parse_skill(const nlohmann::json& j);
    static TargetType parse_target_type(const std::string& s);
    static SkillData::Type parse_executor_type(const std::string& s);
    static DamageFormula parse_dmg_formula(const std::string& s);
    static std::vector<BuffApplication> parse_buffs(const nlohmann::json& j);
};