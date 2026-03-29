#include "act.h"
#include "entity.h"          // 需要 character 的完整定义
#include "customio.h"
#include <algorithm>
#include <climits>
#include <math.h>

using namespace customio;
//前置声明
class Crit;

// ---------- 基类 act 实现 ----------
bool act::hasSufficientResources(const character* c) const {
    for (const auto& pair : consume_) {
        if (c->get_attribute(pair.first) < pair.second) return false;
    }
    return true;
}

bool act::checkProbability() const {
    return chance(probability_);
}

bool act::can_execute(const character* c, const FightContext& ctx) const {
    return hasSufficientResources(c) && checkProbability();
}

// ---------- 攻击行为 ----------
Attack::Attack() : act(80) {
    name_ = "攻击";
    set_consume("C_AP", 17);
}

bool Attack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();   // 需要有敌人
}

bool Attack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择目标
    character* target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!target) return false;

    // 输出行动信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << c->get_name() << " 攻击 " << target->get_name() << "！"
            << resetcolor() << std::endl;
    }

    // 命中判定
    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                << resetcolor() << std::endl;
        }
        return true;
    }

    // 基础伤害
    int atk = c->get_attribute("ATK");
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击处理
    int final_damage = CritCalculator::apply(c, base_damage);

    target->take_damage(final_damage);
    return true;
}
//冲撞
Smash::Smash() : act(40) { name_ = "冲撞"; set_consume("C_AP", 23); }

bool Smash::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();   // 需要有敌人
}

bool Smash::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择目标
    character* target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!target) return false;

    // 输出行动信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::yellow) << c->get_name() << " 撞击 " << target->get_name() << "！"
            << resetcolor() << std::endl;
    }

    // 基础伤害
    int powdamage = std::pow(1.1f, c->get_attribute("ATK"));    //倍率伤害

    int atk = std::max(powdamage, c->get_attribute("ATK"));
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击处理
    int final_damage = CritCalculator::apply(c, base_damage);

    target->take_damage(final_damage);
    return true;
}

//连击
ComboAttack::ComboAttack() : act(70) {  // 概率 70%
    name_ = "连击";
    set_consume("C_AP", 25);            // 消耗 25 AP
}

bool ComboAttack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool ComboAttack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择目标（可改为每次攻击重新选，这里固定第一个敌人）
    character* target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!target) return false;

    // 连击参数
    const int max_hits = 3;            // 最多3次
    const float damage_decay = 0.6f;   // 每次递减到60%
    int current_damage = c->get_attribute("ATK") - target->get_attribute("DEF");
    if (current_damage < 1) current_damage = 1;

    for (int i = 0; i < max_hits; ++i) {
        // 输出信息
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::green) << c->get_name() << " 连击第" << i + 1 << "段！"
                << resetcolor() << std::endl;
        }

        // 命中判定（每段独立）
        const int hit_rate = 90;
        if (chance(hit_rate)) {
            // 应用暴击（复用你的暴击工具）
            int final_damage = CritCalculator::apply(c, current_damage);
            target->take_damage(final_damage);
            if (!target->is_alive()) break;  // 目标死亡，停止连击
        }
        else {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << "连击中断！"
                    << resetcolor() << std::endl;
            }
            break;
        }

        // 伤害递减
        current_damage = static_cast<int>(current_damage * damage_decay);
        if (current_damage < 1) break;
    }

    return true;
}

//魔法飞弹
MagicMissile::MagicMissile() : act(60) { name_ = "魔法飞弹";  set_consume("MP", 20);     }//暂时先用MP，以后改成C_MP
bool MagicMissile::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();   // 需要有敌人
}

bool MagicMissile::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择目标
    character* target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!target) return false;

    // 输出行动信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << c->get_name() << " 发射魔法飞弹!" 
            << resetcolor() << std::endl;
    }

    // 命中判定
    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                << resetcolor() << std::endl;
        }
        return true;
    }

    // 基础伤害
    int atk = c->get_attribute("MATK") * 1.2; //倍率1.2
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击处理
    int final_damage = CritCalculator::apply(c, base_damage);

    target->take_damage(final_damage);
    return true;
}
//加速术
Speedup::Speedup() : act(40) {name_ = "加速术"; set_consume("MP", 15); }
bool Speedup::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    else return 1;
}
bool Speedup::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }


    // 输出行动信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::cyan) << c->get_name() << "使用加速术！\n" << resetcolor() << std::endl;
    }

    //增加C_AP
    int old_ap = c->get_attribute("C_AP");
    c->setattribute("C_AP", old_ap + 40, false);
    return true;
}

//暴击判定
int CritCalculator::apply(character* attacker, int base_damage) {
    if (!attacker) return base_damage;
    int crit_rate = attacker->get_attribute("CRIT");
    if (customio::chance(crit_rate)) {
        int crit_dmg = attacker->get_attribute("CRIT_D");
        int final_damage = base_damage * crit_dmg / 100;
        if (!attacker->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << customio::textcolor(customio::color::red) << "★ 暴击！"
                << customio::resetcolor() << std::endl;
        }
        return final_damage;
    }
    return base_damage;
}
// ---------- 防御行为 ----------
Defense::Defense() : act(100) {
    name_ = "防御";
    set_consume("C_AP", 10);
}

bool Defense::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Defense::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 设置防御 buff（减伤 50%，临时）
    c->setattribute("Defense_BUFF", 50, false);

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::blue) << c->get_name() << " 采取防御姿态！"
            << resetcolor() << std::endl;
    }
    return true;
}

// ---------- 目标选择器实现 ----------
character* TargetSelector::select_lowest_hp(const std::vector<character*>& enemies) {
    character* target = nullptr;
    int lowest_hp = INT_MAX;
    for (character* e : enemies) {
        if (!e || !e->is_alive()) continue;
        int hp = e->get_attribute("HP");
        if (hp < lowest_hp) {
            lowest_hp = hp;
            target = e;
        }
    }
    return target;
}