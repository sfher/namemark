#include "act.h"
#include "entity.h"          // 需要 character 的完整定义
#include "customio.h"
#include <algorithm>
#include <climits>

using namespace customio;

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

    // 命中判定（基础命中率 90%）
    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                << resetcolor() << std::endl;
        }
        return true;
    }

    // 基础伤害 = 攻击力 - 防御力（至少为1）
    int atk = c->get_attribute("ATK");
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击判定
    int final_damage = base_damage;
    if (chance(c->get_attribute("CRIT"))) {
        int crit_dmg = c->get_attribute("CRIT_D");
        final_damage = base_damage * crit_dmg / 100;
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::red) << "★ 暴击！" << resetcolor() << std::endl;
        }
    }

    target->take_damage(final_damage);
    return true;
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