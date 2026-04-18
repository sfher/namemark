// skill_executor.h
#pragma once

#include "entity.h"
#include "act.h"
#include "damage_calculator.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

// Buff 应用描述结构
struct BuffApplication {
    std::string buff_name;  // Buff 名称（如 "POISON", "BURN", "DEFENSE_BUFF"）
    int effect;             // 效果值（伤害/治疗量/百分比）
    int duration;           // 持续回合数
};

// 技能执行器：封装标准技能执行流程
class SkillExecutor {
public:
    // ---------- 通用辅助函数 ----------
    // 扣除技能消耗（修改 caster 属性）
    static bool deduct_cost(
        character* caster,
        const std::unordered_map<std::string, int>& consume,
        const FightContext& ctx
    );

    // 获取目标（调用 act::get_targets 的静态版本）
    static std::vector<character*> get_targets(
        const FightContext& ctx,
        character* caster,
        TargetType target_type
    );

    // 输出行动日志（根据主题）
    static void log_action(character* caster, const std::string& action_name);

    // 命中判定
    static bool hit_check(character* caster, int hit_rate);

    // ---------- 标准伤害技能执行器 ----------
    // 单体伤害技能
    // 返回值：是否成功执行
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
        std::function<void(character*, character*, int)> on_hit_callback = nullptr
    );

    // 群体伤害技能
    static bool execute_aoe_damage(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        TargetType target_type,         // 通常为 ALL_ENEMIES
        int hit_rate,
        DamageFormula dmg_formula,
        float dmg_coeff,
        const std::string& action_name,
        bool use_def = true,
        const std::vector<BuffApplication>& on_hit_buffs = {},
        bool split_damage = false,      // true: 总伤害分摊，false: 各自计算
        std::function<void(character*, character*, int)> on_hit_callback = nullptr
    );

    // ---------- 标准治疗技能执行器 ----------
    static bool execute_heal(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        TargetType target_type,
        const std::string& action_name,
        std::function<int(character*, character*)> heal_amount_func  // 返回治疗量
    );

    // ---------- 标准自我增益技能执行器 ----------
    static bool execute_self_buff(
        character* caster,
        FightContext& ctx,
        const std::unordered_map<std::string, int>& consume,
        const std::string& action_name,
        const std::vector<BuffApplication>& buffs_to_apply,
        const std::unordered_map<std::string, int>& attr_modifiers = {}  // 直接属性修改
    );

    // ---------- 特殊逻辑辅助（供特殊技能调用）----------
    // 施加伤害并附加 Buff（单体）
    static void apply_damage_and_buffs(
        character* caster,
        character* target,
        int damage,
        const std::vector<BuffApplication>& buffs,
        std::function<void(character*, character*, int)> on_hit_callback = nullptr
    );
};