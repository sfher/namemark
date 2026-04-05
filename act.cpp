#include "act.h"
#include "entity.h"
#include "customio.h"
#include <algorithm>
#include <climits>
#include <math.h>

using namespace customio;

std::unordered_map<std::string, SkillInfo> SkillRegistry::skills_;

void SkillRegistry::registerSkill(const std::string& id, const SkillInfo& info) {
    skills_[id] = info;
}

const SkillInfo* SkillRegistry::getSkillInfo(const std::string& id) {
    auto it = skills_.find(id);
    return it != skills_.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string, SkillInfo>& SkillRegistry::getAllSkills() {
    return skills_;
}

void registerAllSkills() {
    SkillRegistry::registerSkill("attack", SkillInfo{ "攻击", 100, 10, []() { return std::make_unique<Attack>(); } });
    SkillRegistry::registerSkill("defense", SkillInfo{ "防御", 80, 5, []() { return std::make_unique<Defense>(); } });
    SkillRegistry::registerSkill("smash", SkillInfo{ "冲撞", 25, 9, []() { return std::make_unique<Smash>(); } });
    SkillRegistry::registerSkill("combo", SkillInfo{ "连击", 30, 7, []() { return std::make_unique<ComboAttack>(); } });
    SkillRegistry::registerSkill("magic_missile", SkillInfo{ "魔法飞弹", 50, 9, []() { return std::make_unique<MagicMissile>(); } });
    SkillRegistry::registerSkill("summon_phantom", SkillInfo{ "召唤幻影", 20, 7, []() { return std::make_unique<SummonPhantom>(); } });
    SkillRegistry::registerSkill("speedup", SkillInfo{ "加速术", 35, 5, []() { return std::make_unique<Speedup>(); } });
    SkillRegistry::registerSkill("poison_magic", SkillInfo{ "毒魔法", 45, 8, []() { return std::make_unique<PoisonMagic>(); } });
    SkillRegistry::registerSkill("fireball", SkillInfo{ "火球术", 55, 9, []() { return std::make_unique<Fireball>(); } });
    SkillRegistry::registerSkill("heal_magic", SkillInfo{ "治愈魔法", 51, 8, []() { return std::make_unique<HealMagic>(); } });
    SkillRegistry::registerSkill("magic_punch", SkillInfo{ "魔法重拳", 52, 8, []() { return std::make_unique<MagicPunch>(); } });
    SkillRegistry::registerSkill("ground_fissure", SkillInfo{ "裂地术", 43, 7, []() { return std::make_unique<GroundFissure>(); } });
    SkillRegistry::registerSkill("lightning_chain", SkillInfo{ "闪电术", 32, 6, []() { return std::make_unique<LightningChain>(); } });
    SkillRegistry::registerSkill("meteor_strike", SkillInfo{ "陨石冲击", 7, 10, []() { return std::make_unique<MeteorStrike>(); } });
    SkillRegistry::registerSkill("void_explosion", SkillInfo{ "虚爆", 7, 10, []() { return std::make_unique<VoidExplosion>(); } });
}
// ---------- 基类 act 实现 ----------
std::vector<character*> act::get_targets(const FightContext& ctx, character* caster) const {
    std::vector<character*> result;
    switch (target_type_) {
    case TargetType::SINGLE_ENEMY_LOWEST_HP: {
        character* target = TargetSelector::select_lowest_hp(ctx.enemies);
        if (target) result.push_back(target);
        break;
    }
    case TargetType::SINGLE_ENEMY_RANDOM: {
        if (!ctx.enemies.empty()) {
            static std::mt19937& rng = customio::get_random_engine();
            std::uniform_int_distribution<size_t> dist(0, ctx.enemies.size() - 1);
            result.push_back(ctx.enemies[dist(rng)]);
        }
        break;
    }
    case TargetType::SINGLE_ENEMY_HIGHEST_ATK: {
        character* target = nullptr;
        int max_atk = -1;
        for (character* e : ctx.enemies) {
            if (e && e->is_alive()) {
                int atk = e->get_attribute("ATK");
                if (atk > max_atk) {
                    max_atk = atk;
                    target = e;
                }
            }
        }
        if (target) result.push_back(target);
        break;
    }
    case TargetType::SINGLE_ALLY_LOWEST_HP: {
        character* target = nullptr;
        int lowest_hp = INT_MAX;
        for (character* ally : ctx.allies) {
            if (ally && ally->is_alive()) {
                int hp = ally->get_attribute("HP");
                if (hp < lowest_hp) {
                    lowest_hp = hp;
                    target = ally;
                }
            }
        }
        if (target) result.push_back(target);
        break;
    }
    case TargetType::SINGLE_ALLY_SELF: {
        if (caster && caster->is_alive()) result.push_back(caster);
        break;
    }
    case TargetType::ALL_ENEMIES: {
        for (character* e : ctx.enemies) {
            if (e && e->is_alive()) result.push_back(e);
        }
        break;
    }
    case TargetType::ALL_ALLIES: {
        for (character* ally : ctx.allies) {
            if (ally && ally->is_alive()) result.push_back(ally);
        }
        break;
    }
    default:
        break;
    }
    return result;
}

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
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool Attack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Attack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << c->get_name() << " 攻击 " << target->get_name() << "！"
                  << resetcolor() << std::endl;
    }

    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                      << resetcolor() << std::endl;
        }
        return true;
    }

    int atk = c->get_attribute("ATK");
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    return true;
}

// 冲撞
Smash::Smash() : act(40) {
    name_ = "冲撞";
    set_consume("C_AP", 23);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool Smash::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Smash::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::yellow) << c->get_name() << " 撞击 " << target->get_name() << "！"
                  << resetcolor() << std::endl;
    }

    float powdamage = std::min((float)std::pow(1.1f, c->get_attribute("ATK")), c->get_attribute("ATK") * 2.5f);
    int atk = std::max((int)powdamage, c->get_attribute("ATK"));
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    return true;
}

// 连击
ComboAttack::ComboAttack() : act(70) {
    name_ = "连击";
    set_consume("C_AP", 25);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool ComboAttack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool ComboAttack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    const int max_hits = 3;
    const float damage_decay = 0.6f;
    int current_damage = c->get_attribute("ATK") - target->get_attribute("DEF");
    if (current_damage < 1) current_damage = 1;

    for (int i = 0; i < max_hits; ++i) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::green) << c->get_name() << " 连击第" << i + 1 << "段！"
                      << resetcolor() << std::endl;
        }

        const int hit_rate = 90;
        if (chance(hit_rate)) {
            int final_damage = CritCalculator::apply(c, current_damage);
            target->take_damage(final_damage);
            if (!target->is_alive()) break;
        } else {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << "连击中断！" << resetcolor() << std::endl;
            }
            break;
        }

        current_damage = static_cast<int>(current_damage * damage_decay);
        if (current_damage < 1) break;
    }
    return true;
}

// 魔法飞弹
MagicMissile::MagicMissile() : act(60) {
    name_ = "魔法飞弹";
    set_consume("MP", 20);
    set_consume("C_AP", 10);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool MagicMissile::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicMissile::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << c->get_name() << " 发射魔法飞弹!"
                  << resetcolor() << std::endl;
    }

    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                      << resetcolor() << std::endl;
        }
        return true;
    }

    int atk = c->get_attribute("MATK") * 2.0f;
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    return true;
}

// 召唤幻影
SummonPhantom::SummonPhantom() : act(35) {
    name_ = "召唤幻影";
    set_consume("MP", 20);
    set_consume("C_AP", 20);
    set_target_type(TargetType::NONE);
}

bool SummonPhantom::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    if (ctx.summoned == nullptr) return false;
    return ctx.char_team.find(const_cast<character*>(c)) != ctx.char_team.end();
}

bool SummonPhantom::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    auto it = ctx.char_team.find(c);
    if (it == ctx.char_team.end() || ctx.summoned == nullptr) return false;
    Team* team = const_cast<Team*>(it->second);

    std::string phantom_name = c->get_name() + "的幻影";
    auto phantom = std::make_unique<character>();
    phantom->set_name(phantom_name);

    int hp = std::max(1, c->get_attribute("HP") * 60 / 100);
    int mp = std::max(0, c->get_attribute("MP") * 50 / 100);
    int atk = std::max(1, c->get_attribute("ATK") * 70 / 100);
    int matk = std::max(1, c->get_attribute("MATK") * 70 / 100);
    int def = std::max(1, c->get_attribute("DEF") * 60 / 100);
    int spd = std::max(1, c->get_attribute("SPD") * 90 / 100);
    int crit = c->get_attribute("CRIT");
    int crit_d = c->get_attribute("CRIT_D");
    int cap = std::max(10, c->get_attribute("C_AP") - 5);

    phantom->setattribute("HP", hp);
    phantom->setattribute("MAX_HP", hp);
    phantom->setattribute("MP", mp);
    phantom->setattribute("ATK", atk);
    phantom->setattribute("MATK", matk);
    phantom->setattribute("DEF", def);
    phantom->setattribute("SPD", spd);
    phantom->setattribute("CRIT", crit);
    phantom->setattribute("CRIT_D", crit_d);
    phantom->setattribute("C_AP", cap);

    phantom->get_actions().clear();
    phantom->get_actions().push_back(std::make_unique<Attack>());
    phantom->get_actions().push_back(std::make_unique<Defense>());

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 召唤出 " << phantom_name << "！" << resetcolor() << std::endl;
    }

    ctx.summoned->push_back(std::move(phantom));
    team->add_character(*ctx.summoned->back());

    return true;
}

// 加速术
Speedup::Speedup() : act(40) {
    name_ = "加速术";
    set_consume("MP", 15);
    set_target_type(TargetType::NONE);
}

bool Speedup::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Speedup::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::cyan) << c->get_name() << "使用加速术！\n" << resetcolor();
    }

    int old_ap = c->get_attribute("C_AP");
    c->setattribute("C_AP", old_ap + 30, false);
    return true;
}

// 毒魔法
PoisonMagic::PoisonMagic() : act(65) {
    name_ = "毒魔法";
    set_consume("MP", 25);
    set_consume("C_AP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool PoisonMagic::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool PoisonMagic::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << c->get_name() << " 施放毒魔法！"
                  << resetcolor() << std::endl;
    }

    const int hit_rate = 85;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 的魔法被躲避了！"
                      << resetcolor() << std::endl;
        }
        return true;
    }

    int atk = c->get_attribute("MATK") * 1.5f;
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    target->add_buff("POISON", 8, 4);
    return true;
}

// 火球术
Fireball::Fireball() : act(75) {
    name_ = "火球术";
    set_consume("MP", 30);
    set_consume("C_AP", 18);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool Fireball::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Fireball::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::red) << c->get_name() << " 施放火球术！"
                  << resetcolor() << std::endl;
    }

    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 的魔法被躲避了！"
                      << resetcolor() << std::endl;
        }
        return true;
    }

    int atk = c->get_attribute("MATK") * 2.0f;
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    target->add_buff("BURN", 12, 3);
    return true;
}

// 治愈魔法
HealMagic::HealMagic() : act(80) {
    name_ = "治愈魔法";
    set_consume("MP", 28);
    set_consume("C_AP", 16);
    set_target_type(TargetType::SINGLE_ALLY_LOWEST_HP);
}

bool HealMagic::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.allies.empty();
}

bool HealMagic::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 施放治愈魔法！"
                  << resetcolor() << std::endl;
    }

    int heal_amount = static_cast<int>(c->get_attribute("MATK") * 1.8f + 50);
    int current_hp = target->get_attribute("HP");
    int max_hp = target->get_attribute("MAX_HP");
    int new_hp = std::min(current_hp + heal_amount, max_hp);
    int actual_heal = new_hp - current_hp;
    target->setattribute("HP", new_hp, false);

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::green) << target->get_name() << " 恢复了 "
                  << actual_heal << " 点生命值！当前HP: " << new_hp << "/"
                  << max_hp << resetcolor() << std::endl;
    }
    return true;
}

// 魔法重拳
MagicPunch::MagicPunch() : act(40) {
    name_ = "魔法重拳";
    set_consume("MP", 12);
    set_consume("C_AP", 12);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool MagicPunch::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicPunch::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::red) << c->get_name() << " 使用魔法重拳!"
                  << resetcolor() << std::endl;
    }

    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 攻击落空了！"
                      << resetcolor() << std::endl;
        }
        return true;
    }

    int atk = c->get_attribute("MATK") + c->get_attribute("ATK");
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    return true;
}

// 裂地术
GroundFissure::GroundFissure() : act(70) {  // 成功率降低到 70%
    name_ = "裂地术";
    set_consume("C_AP", 35);   // 提高 AP 消耗
    set_consume("MP", 25);     // 提高 MP 消耗
    set_target_type(TargetType::ALL_ENEMIES);
}

bool GroundFissure::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool GroundFissure::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 施放裂地术！"
                  << resetcolor() << std::endl;
    }

    // 伤害分摊：总伤害固定 = MATK * 1.5，人数越多单体越低
    int total_damage = static_cast<int>(c->get_attribute("MATK") * 1.5f);
    int per_target_damage = std::max(1, total_damage / (int)targets.size());

    const int hit_rate = 50;  // 命中率降低到 50%
    for (character* target : targets) {
        if (!chance(hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << " 裂地术未能击中 " << target->get_name()
                          << resetcolor() << std::endl;
            }
            continue;
        }
        int final_damage = CritCalculator::apply(c, per_target_damage);
        target->take_damage(final_damage);
    }
    return true;
}
// 闪电术
LightningChain::LightningChain() : act(75) {
    name_ = "闪电术";
    set_consume("C_AP", 15);
    set_consume("MP", 20);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP); // 初始目标策略
}

bool LightningChain::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool LightningChain::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* current_target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::cyan) << c->get_name() << " 施放闪电术！"
                  << resetcolor() << std::endl;
    }

    int base_damage = static_cast<int>(c->get_attribute("MATK") * 2.0f);
    const float decay_factor = 0.7f;
    const int max_jumps = 3;
    std::vector<character*> hit_targets;
    hit_targets.push_back(current_target);

    for (int jump = 0; jump < max_jumps; ++jump) {
        std::vector<character*> available;
        for (character* enemy : ctx.enemies) {
            if (std::find(hit_targets.begin(), hit_targets.end(), enemy) == hit_targets.end()) {
                available.push_back(enemy);
            }
        }
        if (available.empty()) break;

        static std::mt19937& rng = customio::get_random_engine();
        std::uniform_int_distribution<int> dist(0, available.size() - 1);
        character* next_target = available[dist(rng)];
        hit_targets.push_back(next_target);
    }

    for (size_t idx = 0; idx < hit_targets.size(); ++idx) {
        character* target = hit_targets[idx];
        float multiplier = std::pow(decay_factor, idx);
        int damage = static_cast<int>(base_damage * multiplier);
        if (damage < 1) damage = 1;

        const int hit_rate = 85;
        if (!chance(hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << " 闪电击中 " << target->get_name()
                          << " 但被躲开了！" << resetcolor() << std::endl;
            }
            continue;
        }

        int final_damage = CritCalculator::apply(c, damage);
        target->take_damage(final_damage);

        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << " 闪电传导至 " << target->get_name()
                      << "，造成 " << final_damage << " 点伤害！" << resetcolor() << std::endl;
        }
    }
    return true;
}

MeteorStrike::MeteorStrike() : act(50) {
    name_ = "陨石打击";
    set_consume("MP", 40);
    set_consume("C_AP", 30);
    set_target_type(TargetType::ALL_ENEMIES);
}
bool MeteorStrike::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MeteorStrike::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::red) << c->get_name() << " 施放陨石打击！"
            << resetcolor() << std::endl;
    }

	int base_damage = static_cast<int>(c->get_attribute("MATK") * 3.0f); // 强力单体伤害，后续可调整为群体伤害
    int per_target_damage = base_damage; // 可根据人数调整

    for (character* target : targets) {
        if (!chance(85)) continue;
        int final_damage = CritCalculator::apply(c, per_target_damage);
        target->take_damage(final_damage);
    }
    return true;
}

VoidExplosion::VoidExplosion() : act(40) {
    name_ = "虚爆";
    set_consume("MP", 20);
    set_consume("C_AP", 20);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool VoidExplosion::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    if (c->get_attribute("AP") % 2 == 0 && c->get_attribute("MP") % 2 == 0) //如果AP和MP都是偶数触发
    {
        return !ctx.enemies.empty();
    }
    else return 0;
}

bool VoidExplosion::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    std::vector<character*> targets = get_targets(ctx, c);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::red) << c->get_name() << " 使用虚爆!"
            << resetcolor() << std::endl;
    }

	int atk = std::pow((c->get_attribute("MATK") + c->get_attribute("ATK")), 1.2); //虚爆伤害基于攻击和魔攻的综合，且有额外的倍率加成
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);
    return true;
}

// 暴击判定
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

// 防御行为
Defense::Defense() : act(20) {
    name_ = "防御";
    set_consume("C_AP", 20);
    set_target_type(TargetType::NONE);
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

    const_cast<character*>(c)->add_buff("DEFENSE_BUFF", 50, 1);

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::blue) << c->get_name() << " 采取防御姿态！"
                  << resetcolor() << std::endl;
    }
    return true;
}

// 目标选择器
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