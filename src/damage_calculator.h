// damage_calculator.h
#pragma once

#include "entity.h"   // for character
#include <functional>

// 伤害计算公式枚举
enum class DamageFormula {
    ATK,                // 物理攻击力
    MATK,               // 魔法攻击力
    ATK_MATK_SUM,       // 物攻+魔攻
    ATK_MATK_COMBINED,  // (ATK + MATK) * 系数
    ATK_POWER,          // min(1.1^ATK, ATK*2.5)  (Smash专用)
    MATK_POWER,         // MATK的幂次 (未使用但预留)
    CUSTOM              // 自定义函数
};

// 伤害计算器
class DamageCalculator {
public:
    // 计算原始攻击力（防御减免前）
    // caster: 攻击者
    // formula: 公式类型
    // coeff: 倍率系数（默认1.0）
    // custom_func: 当formula为CUSTOM时使用的自定义函数
    static int calculate_raw_attack(
        const character* caster,
        DamageFormula formula,
        float coeff = 1.0f,
        std::function<int(const character*)> custom_func = nullptr
    );

    // 计算最终伤害（包含防御减免和暴击）
    // caster: 攻击者
    // target: 目标
    // raw_atk: 原始攻击力（由calculate_raw_attack得到）
    // use_def: 是否应用目标防御（默认true）
    // 返回值: 最终伤害值（已应用暴击）
    static int calculate_final_damage(
        character* caster,
        const character* target,
        int raw_atk,
        bool use_def = true
    );

    // 便捷组合函数：一步计算伤害
    static int calculate_damage(
        character* caster,
        const character* target,
        DamageFormula formula,
        float coeff = 1.0f,
        bool use_def = true
    );
};