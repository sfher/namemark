// damage_calculator.cpp
#include "damage_calculator.h"
#include "act.h"        // for CritCalculator
#include <algorithm>
#include <cmath>

int DamageCalculator::calculate_raw_attack(
    const character* caster,
    DamageFormula formula,
    float coeff,
    std::function<int(const character*)> custom_func)
{
    if (!caster) return 0;

    switch (formula) {
    case DamageFormula::ATK:
        return static_cast<int>(caster->get_attribute("ATK") * coeff);

    case DamageFormula::MATK:
        return static_cast<int>(caster->get_attribute("MATK") * coeff);

    case DamageFormula::ATK_MATK_SUM:
        return static_cast<int>((caster->get_attribute("ATK") + caster->get_attribute("MATK")) * coeff);

    case DamageFormula::ATK_MATK_COMBINED:
        // 默认实现，具体系数由coeff控制
        return static_cast<int>((caster->get_attribute("ATK") + caster->get_attribute("MATK")) * coeff);

    case DamageFormula::ATK_POWER: {
        // 保留Smash的原有效果: min(1.1^ATK, ATK*2.5)
        int atk = caster->get_attribute("ATK");
        double power_val = std::pow(1.1, atk);
        double cap = atk * 2.5;
        return static_cast<int>(std::min(power_val, cap));
    }

    case DamageFormula::MATK_POWER: {
        // 预留，暂不使用
        int matk = caster->get_attribute("MATK");
        double power_val = std::pow(1.1, matk);
        double cap = matk * 2.5;
        return static_cast<int>(std::min(power_val, cap));
    }

    case DamageFormula::CUSTOM:
        if (custom_func) {
            return custom_func(caster);
        }
        return 0;

    default:
        return 0;
    }
}

int DamageCalculator::calculate_final_damage(
    character* caster,
    const character* target,
    int raw_atk,
    bool use_def)
{
    if (!caster || !target) return 0;
    if (raw_atk <= 0) return 1;  // 至少造成1点伤害

    int base_damage = raw_atk;
    if (use_def) {
        int def = target->get_attribute("DEF");
        base_damage = std::max(1, raw_atk - def);
    }

    // 应用暴击
    return CritCalculator::apply(caster, base_damage);
}

int DamageCalculator::calculate_damage(
    character* caster,
    const character* target,
    DamageFormula formula,
    float coeff,
    bool use_def)
{
    int raw = calculate_raw_attack(caster, formula, coeff);
    return calculate_final_damage(caster, target, raw, use_def);
}