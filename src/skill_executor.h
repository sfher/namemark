// skill_executor.h
#pragma once

#include "entity.h"
#include "act.h"
#include "damage_calculator.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

struct BuffApplication {
    std::string buff_name;
    int effect;
    int duration;
};

class SkillExecutor {
public:
    static bool deduct_cost(
        character* caster,
        const std::unordered_map<std::string, int>& consume,
        const FightContext& ctx
    );

    static std::vector<character*> get_targets(
        const FightContext& ctx,
        character* caster,
        TargetType target_type
    );

    // 修改：增加 description 参数
    static void log_action(character* caster, const std::string& action_name, const std::string& description = "");

    static bool hit_check(character* caster, int hit_rate);

    static bool execute_single_target_damage(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        TargetType target_type,
        int hit_rate,
        DamageFormula dmg_formula,
        float dmg_coeff,
        const std::string& action_name,
        bool use_def = true,
        const std::vector<BuffApplication>& on_hit_buffs = {},
        std::function<void(character*, character*, int)> on_hit_callback = nullptr,
        const std::string& description = ""       // 新增
    );

    static bool execute_aoe_damage(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        TargetType target_type,
        int hit_rate,
        DamageFormula dmg_formula,
        float dmg_coeff,
        const std::string& action_name,
        bool use_def = true,
        const std::vector<BuffApplication>& on_hit_buffs = {},
        bool split_damage = false,
        std::function<void(character*, character*, int)> on_hit_callback = nullptr,
        const std::string& description = ""       // 新增
    );

    static bool execute_heal(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        TargetType target_type,
        const std::string& action_name,
        std::function<int(character*, character*)> heal_amount_func,
        const std::string& description = ""       // 新增
    );

    static bool execute_self_buff(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        const std::string& action_name,
        const std::vector<BuffApplication>& buffs_to_apply,
        const std::unordered_map<std::string, int>& attr_modifiers = {},
        const std::string& description = ""       // 新增
    );

    static void apply_damage_and_buffs(
        character* caster,
        character* target,
        int damage,
        const std::vector<BuffApplication>& buffs,
        std::function<void(character*, character*, int)> on_hit_callback = nullptr
    );
};