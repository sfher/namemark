// skill_data.h
#pragma once

#include "act.h"
#include "damage_calculator.h"
#include "skill_executor.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

// 技能数据配置（用于数据驱动）
struct SkillData {
    std::string id;                 // 内部 ID（用于注册）
    std::string display_name;       // 显示名称
    int acquire_chance = 100;       // 习得概率
    int weight = 5;                 // AI 选择权重
    int base_probability = 100;     // 技能成功率
    std::string description;        // 技能描述

    std::unordered_map<std::string, int> consume;
    TargetType target_type = TargetType::SINGLE_ENEMY_LOWEST_HP;

    // 执行器类型
    enum class Type {
        SINGLE_DAMAGE,
        AOE_DAMAGE,
        HEAL,
        SELF_BUFF,
        CUSTOM
    } type = Type::CUSTOM;

    // 伤害/治疗参数
    int hit_rate = 100;
    DamageFormula dmg_formula = DamageFormula::ATK;
    float dmg_coeff = 1.0f;
    bool use_def = true;
    bool split_damage = false;      // 仅 AOE 有效

    // Buff 列表
    std::vector<BuffApplication> on_hit_buffs;
    std::vector<BuffApplication> self_buffs;
    std::unordered_map<std::string, int> attr_modifiers;

    // 治疗量计算函数（仅 HEAL 类型使用）
    std::function<int(character*, character*)> heal_func;

    // 自定义工厂（仅 CUSTOM 类型使用）
    std::function<std::unique_ptr<act>()> custom_factory;
};