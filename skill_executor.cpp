// skill_executor.cpp
#define NOMINMAX
#include <Windows.h>    // for Sleep

#include "skill_executor.h"   // 必须包含自己的头文件
#include "act.h"              // 提供 act 类的完整定义（用于 TempAct）
#include "damage_calculator.h"
#include "customio.h"
#include "entity.h"

#include <iostream>
#include <algorithm>
#include <functional>

using namespace customio;

// ... 其余实现不变

// ---------- 通用辅助函数实现 ----------

bool SkillExecutor::deduct_cost(
    character* caster,
    const std::unordered_map<std::string, int>& consume,
    const FightContext& ctx)
{
    if (!caster) return false;
    for (const auto& pair : consume) {
        int old = caster->get_attribute(pair.first);
        if (old < pair.second) {
            // 资源不足，不应发生（can_execute 已检查）
            return false;
        }
        caster->setattribute(pair.first, old - pair.second, false);
    }
    return true;
}

std::vector<character*> SkillExecutor::get_targets(
    const FightContext& ctx,
    character* caster,
    TargetType target_type)
{
    // 临时创建一个 act 对象来调用 get_targets（或重构为静态函数）
    // 简便方法：创建一个临时 act 对象，设置目标类型后调用
    class TempAct : public act {
    public:
        TempAct(TargetType t) { set_target_type(t); }
        bool execute(character*, FightContext&) override { return true; }
        using act::get_targets;  // 暴露 protected 方法
    };
    TempAct temp(target_type);
    return temp.get_targets(ctx, caster);
}

void SkillExecutor::log_action(character* caster, const std::string& action_name) {
    if (!caster || caster->GetRule(BATTLE_WITHOUT_OUTPUT)) return;
    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.attack)
        << caster->get_name() << " 使用了 " << action_name << "！"
        << resetcolor() << std::endl;
    Sleep(150);
}

bool SkillExecutor::hit_check(character* caster, int hit_rate) {
    if (hit_rate >= 100) return true;
    if (hit_rate <= 0) return false;
    return customio::chance(hit_rate);
}

void SkillExecutor::apply_damage_and_buffs(
    character* caster,
    character* target,
    int damage,
    const std::vector<BuffApplication>& buffs,
    std::function<void(character*, character*, int)> on_hit_callback)
{
    if (!caster || !target) return;

    // 造成伤害
    target->take_damage(damage);

    // 施加 Buff
    for (const auto& buff : buffs) {
        target->add_buff(buff.buff_name, buff.effect, buff.duration);
    }

    // 自定义回调（例如吸血、额外效果）
    if (on_hit_callback) {
        on_hit_callback(caster, target, damage);
    }
}

// ---------- 单体伤害技能执行器 ----------

bool SkillExecutor::execute_single_target_damage(
    character* caster,
    FightContext& ctx,
    const std::unordered_map<std::string, int>& consume,
    TargetType target_type,
    int hit_rate,
    DamageFormula dmg_formula,
    float dmg_coeff,
    const std::string& action_name,
    bool use_def,
    const std::vector<BuffApplication>& on_hit_buffs,
    std::function<void(character*, character*, int)> on_hit_callback)
{
    if (!caster) return false;

    // 1. 扣费
    if (!deduct_cost(caster, consume, ctx)) return false;

    // 2. 索敌
    std::vector<character*> targets = get_targets(ctx, caster, target_type);
    if (targets.empty()) return false;
    character* target = targets[0];

    // 3. 日志
    log_action(caster, action_name);

    // 4. 命中判定
    if (!hit_check(caster, hit_rate)) {
        if (!caster->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            const auto& theme = get_console_theme();
            std::cout << adaptive_textcolor(theme.warning)
                << caster->get_name() << " 的攻击落空了！"
                << resetcolor() << std::endl;
        }
        return true;  // 消耗了资源但未命中，仍算执行成功
    }

    // 5. 伤害计算
    int raw_atk = DamageCalculator::calculate_raw_attack(caster, dmg_formula, dmg_coeff);
    int final_damage = DamageCalculator::calculate_final_damage(caster, target, raw_atk, use_def);

    // 6. 施加伤害与 Buff
    apply_damage_and_buffs(caster, target, final_damage, on_hit_buffs, on_hit_callback);

    return true;
}

// ---------- 群体伤害技能执行器 ----------

bool SkillExecutor::execute_aoe_damage(
    character* caster,
    FightContext& ctx,
    const std::unordered_map<std::string, int>& consume,
    TargetType target_type,
    int hit_rate,
    DamageFormula dmg_formula,
    float dmg_coeff,
    const std::string& action_name,
    bool use_def,
    const std::vector<BuffApplication>& on_hit_buffs,
    bool split_damage,
    std::function<void(character*, character*, int)> on_hit_callback)
{
    if (!caster) return false;

    // 1. 扣费
    if (!deduct_cost(caster, consume, ctx)) return false;

    // 2. 索敌
    std::vector<character*> targets = get_targets(ctx, caster, target_type);
    if (targets.empty()) return false;

    // 3. 日志
    log_action(caster, action_name);

    // 4. 计算原始总伤害（若分摊）
    int total_raw_atk = 0;
    if (split_damage) {
        total_raw_atk = DamageCalculator::calculate_raw_attack(caster, dmg_formula, dmg_coeff);
    }

    // 5. 遍历目标
    for (character* target : targets) {
        if (!target->is_alive()) continue;

        // 命中判定
        if (!hit_check(caster, hit_rate)) {
            if (!caster->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                const auto& theme = get_console_theme();
                std::cout << adaptive_textcolor(theme.warning)
                    << action_name << " 未能击中 " << target->get_name() << "！"
                    << resetcolor() << std::endl;
            }
            continue;
        }

        int final_damage;
        if (split_damage) {
            // 分摊：每个目标受到均分伤害
            int per_target_raw = std::max(1, total_raw_atk / static_cast<int>(targets.size()));
            final_damage = DamageCalculator::calculate_final_damage(caster, target, per_target_raw, use_def);
        }
        else {
            // 独立计算
            int raw = DamageCalculator::calculate_raw_attack(caster, dmg_formula, dmg_coeff);
            final_damage = DamageCalculator::calculate_final_damage(caster, target, raw, use_def);
        }

        apply_damage_and_buffs(caster, target, final_damage, on_hit_buffs, on_hit_callback);
    }

    return true;
}

// ---------- 治疗技能执行器 ----------

bool SkillExecutor::execute_heal(
    character* caster,
    FightContext& ctx,
    const std::unordered_map<std::string, int>& consume,
    TargetType target_type,
    const std::string& action_name,
    std::function<int(character*, character*)> heal_amount_func)
{
    if (!caster) return false;

    // 1. 扣费
    if (!deduct_cost(caster, consume, ctx)) return false;

    // 2. 索敌
    std::vector<character*> targets = get_targets(ctx, caster, target_type);
    if (targets.empty()) return false;
    character* target = targets[0];

    // 3. 日志
    log_action(caster, action_name);

    // 4. 计算治疗量
    int heal_amount = heal_amount_func(caster, target);
    int current_hp = target->get_attribute("HP");
    int max_hp = target->get_attribute("MAX_HP");
    int new_hp = std::min(current_hp + heal_amount, max_hp);
    int actual_heal = new_hp - current_hp;

    target->setattribute("HP", new_hp, false);

    // 5. 输出治疗日志
    if (!caster->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        const auto& theme = get_console_theme();
        std::cout << adaptive_textcolor(theme.text) << target->get_name() << " 恢复了 "
            << adaptive_textcolor(theme.heal) << actual_heal
            << adaptive_textcolor(theme.text) << " 点生命值！当前HP: "
            << new_hp << "/" << max_hp << resetcolor() << std::endl;
        Sleep(100);
    }

    return true;
}

// ---------- 自我增益技能执行器 ----------

bool SkillExecutor::execute_self_buff(
    character* caster,
    FightContext& ctx,
    const std::unordered_map<std::string, int>& consume,
    const std::string& action_name,
    const std::vector<BuffApplication>& buffs_to_apply,
    const std::unordered_map<std::string, int>& attr_modifiers)
{
    if (!caster) return false;

    // 1. 扣费
    if (!deduct_cost(caster, consume, ctx)) return false;

    // 2. 日志
    log_action(caster, action_name);

    // 3. 应用属性修改
    for (const auto& mod : attr_modifiers) {
        int old = caster->get_attribute(mod.first);
        caster->setattribute(mod.first, old + mod.second, false);
    }

    // 4. 应用 Buff
    for (const auto& buff : buffs_to_apply) {
        caster->add_buff(buff.buff_name, buff.effect, buff.duration);
    }

    return true;
}