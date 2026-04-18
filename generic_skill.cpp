// generic_skill.cpp
#include "generic_skill.h"
#include "skill_executor.h"

GenericSkill::GenericSkill(const SkillData& data)
    : act(data.base_probability), data_(data)
{
    name_ = data.display_name;
    for (const auto& pair : data.consume) {
        set_consume(pair.first, pair.second);
    }
    set_target_type(data.target_type);
}

bool GenericSkill::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    // 额外检查（如敌人是否存在）
    switch (data_.target_type) {
    case TargetType::SINGLE_ENEMY_LOWEST_HP:
    case TargetType::SINGLE_ENEMY_RANDOM:
    case TargetType::SINGLE_ENEMY_HIGHEST_ATK:
    case TargetType::ALL_ENEMIES:
        if (ctx.enemies.empty()) return false;
        break;
    case TargetType::SINGLE_ALLY_LOWEST_HP:
    case TargetType::ALL_ALLIES:
        if (ctx.allies.empty()) return false;
        break;
    default:
        break;
    }
    return true;
}

bool GenericSkill::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    switch (data_.type) {
    case SkillData::Type::SINGLE_DAMAGE:
        return SkillExecutor::execute_single_target_damage(
            c, ctx, consume_, data_.target_type,
            data_.hit_rate, data_.dmg_formula, data_.dmg_coeff,
            name_, data_.use_def, data_.on_hit_buffs, nullptr
        );

    case SkillData::Type::AOE_DAMAGE:
        return SkillExecutor::execute_aoe_damage(
            c, ctx, consume_, data_.target_type,
            data_.hit_rate, data_.dmg_formula, data_.dmg_coeff,
            name_, data_.use_def, data_.on_hit_buffs,
            data_.split_damage, nullptr
        );

    case SkillData::Type::HEAL:
        if (data_.heal_func) {
            return SkillExecutor::execute_heal(
                c, ctx, consume_, data_.target_type,
                name_, data_.heal_func
            );
        }
        return false;

    case SkillData::Type::SELF_BUFF:
        return SkillExecutor::execute_self_buff(
            c, ctx, consume_, name_,
            data_.self_buffs, data_.attr_modifiers
        );

    case SkillData::Type::CUSTOM:
        // CUSTOM 类型不应该通过 GenericSkill 调用
        return false;
    }
    return false;
}