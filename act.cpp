// act.cpp
#include "act.h"
#include "entity.h"
#include "customio.h"
#include "skill_executor.h"
#include "damage_calculator.h"
#include "skill_data.h"
#include "generic_skill.h"
#include <algorithm>
#include <climits>
#include <math.h>
#define NOMINMAX
#include <windows.h> // for Sleep

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
    using SD = SkillData;

    // ---------- 数据驱动的标准技能表 ----------
    std::vector<SkillData> skill_table = {
        // 攻击
        { "attack", "攻击", 100, 10, 80, "用武器进行基础攻击。",
          {{"C_AP", 12}}, TargetType::SINGLE_ENEMY_LOWEST_HP,
          SD::Type::SINGLE_DAMAGE, 90, DamageFormula::ATK, 1.0f, true, false,
          {}, {}, {}, nullptr, nullptr },

          // 防御
          { "defense", "防御", 80, 5, 20, "采取防御姿态，减少50%伤害，持续2回合。",
            {{"C_AP", 15}}, TargetType::NONE,
            SD::Type::SELF_BUFF, 100, DamageFormula::ATK, 0.0f, false, false,
            {}, { {"DEFENSE_BUFF", 50, 2} }, {}, nullptr, nullptr },

            // 魔法飞弹
            { "magic_missile", "魔法飞弹", 50, 9, 60, "发射一枚魔法飞弹，造成魔法伤害。",
              {{"MP", 15}, {"C_AP", 10}}, TargetType::SINGLE_ENEMY_LOWEST_HP,
              SD::Type::SINGLE_DAMAGE, 90, DamageFormula::MATK, 1.6f, true, false,
              {}, {}, {}, nullptr, nullptr },

              // 加速术
              { "speedup", "加速术", 35, 6, 40, "通过魔法加速，获得25点行动点。",
                {{"MP", 20}}, TargetType::NONE,
                SD::Type::SELF_BUFF, 100, DamageFormula::ATK, 0.0f, false, false,
                {}, {}, {{"C_AP", 25}}, nullptr, nullptr },

                // 魔法重拳
                { "magic_punch", "魔法重拳", 60, 10, 40, "将魔力凝聚于拳头，造成混合伤害。",
                  {{"MP", 15}, {"C_AP", 15}}, TargetType::SINGLE_ENEMY_LOWEST_HP,
                  SD::Type::SINGLE_DAMAGE, 90, DamageFormula::ATK_MATK_COMBINED, 1.2f, true, false,
                  {}, {}, {}, nullptr, nullptr },

                  // 虚爆
                  { "void_explosion", "虚爆", 7, 10, 40, "引发虚空爆炸，造成巨额混合伤害。",
                    {{"MP", 25}, {"C_AP", 25}}, TargetType::SINGLE_ENEMY_LOWEST_HP,
                    SD::Type::SINGLE_DAMAGE, 90, DamageFormula::ATK_MATK_COMBINED, 1.8f, true, false,
                    {}, {}, {}, nullptr, nullptr },

                    // 裂地术
                    { "ground_fissure", "裂地术", 30, 7, 70, "撕裂大地，对全体敌人造成魔法伤害。",
                      {{"C_AP", 25}, {"MP", 20}}, TargetType::ALL_ENEMIES,
                      SD::Type::AOE_DAMAGE, 80, DamageFormula::MATK, 1.2f, true, false,
                      {}, {}, {}, nullptr, nullptr },

                      // 陨石打击
                      { "meteor_strike", "陨石打击", 7, 10, 50, "召唤陨石轰击全场，造成毁灭性魔法伤害。",
                        {{"MP", 40}, {"C_AP", 30}}, TargetType::ALL_ENEMIES,
                        SD::Type::AOE_DAMAGE, 75, DamageFormula::MATK, 3.0f, true, false,
                        {}, {}, {}, nullptr, nullptr },

                        // === 新增技能 ===
                        // 重伤打击
                        { "heavy_strike", "重伤打击", 30, 7, 50, "舍弃命中率，全力一击。",
                          {{"C_AP", 30}}, TargetType::SINGLE_ENEMY_LOWEST_HP,
                          SD::Type::SINGLE_DAMAGE, 60, DamageFormula::ATK, 2.5f, true, false,
                          {}, {}, {}, nullptr, nullptr },

                          // 石肤术
                          { "stone_skin", "石肤术", 25, 6, 60, "皮肤化为岩石，受到伤害降低70%，持续3回合。",
                            {{"MP", 20}, {"C_AP", 15}}, TargetType::NONE,
                            SD::Type::SELF_BUFF, 100, DamageFormula::ATK, 0.0f, false, false,
                            {}, { {"DEFENSE_BUFF", 70, 3} }, {}, nullptr, nullptr },
    };

    // 注册数据表技能
    for (const auto& data : skill_table) {
        SkillInfo info(
            data.display_name,
            data.acquire_chance,
            data.weight,
            [data]() -> std::unique_ptr<act> {
                return std::make_unique<GenericSkill>(data);
            }
        );
        SkillRegistry::registerSkill(data.id, info);
    }

    // 注册保留的派生类技能
    SkillRegistry::registerSkill("smash", SkillInfo("冲撞", 40, 8, []() { return std::make_unique<Smash>(); }));
    SkillRegistry::registerSkill("combo", SkillInfo("连击", 30, 7, []() { return std::make_unique<ComboAttack>(); }));
    SkillRegistry::registerSkill("poison_magic", SkillInfo("毒魔法", 45, 8, []() { return std::make_unique<PoisonMagic>(); }));
    SkillRegistry::registerSkill("fireball", SkillInfo("火球术", 55, 9, []() { return std::make_unique<Fireball>(); }));
    SkillRegistry::registerSkill("heal_magic", SkillInfo("治愈魔法", 50, 8, []() { return std::make_unique<HealMagic>(); }));
    SkillRegistry::registerSkill("summon_phantom", SkillInfo("召唤幻影", 25, 9, []() { return std::make_unique<SummonPhantom>(); }));
    SkillRegistry::registerSkill("lightning", SkillInfo("闪电术", 20, 8, []() { return std::make_unique<LightningChain>(); }));
    SkillRegistry::registerSkill("self_heal", SkillInfo("自愈", 7, 10, []() { return std::make_unique<SelfHeal>(); }));
    SkillRegistry::registerSkill("void_god_summon", SkillInfo("虚神召唤", 10, 10, []() { return std::make_unique<VoidGodSummon>(); }));
    SkillRegistry::registerSkill("lifesteal", SkillInfo("吸血", 20, 8, []() { return std::make_unique<Lifesteal>(); }));
    SkillRegistry::registerSkill("meditate", SkillInfo("冥想", 30, 5, []() { return std::make_unique<Meditate>(); }));
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
    set_consume("C_AP", 12);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool Attack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Attack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        90,
        DamageFormula::ATK,
        1.0f,
        name_,
        true,
        {},
        nullptr
    );
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

    if (!SkillExecutor::deduct_cost(c, consume_, ctx)) return false;

    std::vector<character*> targets = SkillExecutor::get_targets(ctx, c, target_type_);
    if (targets.empty()) return false;
    character* target = targets[0];

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        const auto& theme = customio::get_console_theme();
        std::cout << customio::adaptive_textcolor(theme.attack)
            << c->get_name() << " 撞击 " << target->get_name() << "！"
            << customio::resetcolor() << std::endl;
        Sleep(150);
    }

    int raw_atk = DamageCalculator::calculate_raw_attack(c, DamageFormula::ATK_POWER);
    int final_damage = DamageCalculator::calculate_final_damage(c, target, raw_atk, true);

    target->take_damage(final_damage);
    return true;
}

// 连击
ComboAttack::ComboAttack() : act(70) {
    name_ = "连击";
    set_consume("C_AP", 22);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool ComboAttack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool ComboAttack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    if (!SkillExecutor::deduct_cost(c, consume_, ctx)) return false;

    std::vector<character*> targets = SkillExecutor::get_targets(ctx, c, target_type_);
    if (targets.empty()) return false;
    character* target = targets[0];

    const int max_hits = 3;
    const float damage_coeff = 0.8f;
    const float decay = 0.6f;
    const int hit_rate = 90;

    for (int i = 0; i < max_hits; ++i) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << customio::textcolor(customio::color::white)
                << c->get_name() << " 连击第" << i + 1 << "段！"
                << customio::resetcolor() << std::endl;
            Sleep(100);
        }

        if (!SkillExecutor::hit_check(c, hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << customio::textcolor(customio::color::yellow)
                    << "连击未命中！" << customio::resetcolor() << std::endl;
            }
        }
        else {
            float coeff = damage_coeff * std::pow(decay, i);
            int raw_atk = DamageCalculator::calculate_raw_attack(c, DamageFormula::ATK, coeff);
            int final_damage = DamageCalculator::calculate_final_damage(c, target, raw_atk, true);
            target->take_damage(final_damage);
            if (!target->is_alive()) break;
        }
    }
    return true;
}

// 魔法飞弹
MagicMissile::MagicMissile() : act(60) {
    name_ = "魔法飞弹";
    set_consume("MP", 15);
    set_consume("C_AP", 10);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool MagicMissile::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicMissile::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        90,
        DamageFormula::MATK,
        1.6f,
        name_,
        true
    );
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

    phantom->SetRule(SLOW_BATTLE, c->GetRule(SLOW_BATTLE));
    phantom->SetRule(BATTLE_WITHOUT_OUTPUT, c->GetRule(BATTLE_WITHOUT_OUTPUT));

    phantom->get_actions().clear();
    phantom->get_actions().push_back(std::make_unique<Attack>());
    phantom->get_actions().push_back(std::make_unique<Defense>());

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::white) << c->get_name() << " 召唤出 " << phantom_name << "！" << resetcolor() << std::endl;
        Sleep(150);
    }

    ctx.summoned->push_back(std::move(phantom));
    team->add_character(*ctx.summoned->back());

    return true;
}

// 加速术
Speedup::Speedup() : act(40) {
    name_ = "加速术";
    set_consume("MP", 20);
    set_target_type(TargetType::NONE);
}

bool Speedup::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Speedup::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    std::unordered_map<std::string, int> attr_mods = {
        {"C_AP", 25}
    };

    return SkillExecutor::execute_self_buff(
        c, ctx, consume_,
        name_,
        {},
        attr_mods
    );
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

    int poison_damage = static_cast<int>(c->get_attribute("MATK") * 0.2f);
    std::vector<BuffApplication> buffs = {
        {"POISON", poison_damage, 3}
    };

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        85,
        DamageFormula::MATK,
        1.5f,
        name_,
        true,
        buffs
    );
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

    int burn_damage = static_cast<int>(c->get_attribute("MATK") * 0.3f);
    std::vector<BuffApplication> buffs = {
        {"BURN", burn_damage, 2}
    };

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        90,
        DamageFormula::MATK,
        2.0f,
        name_,
        true,
        buffs
    );
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

    auto heal_func = [](character* caster, character* target) -> int {
        return static_cast<int>(caster->get_attribute("MATK") * 2.0f + 30);
        };

    return SkillExecutor::execute_heal(
        c, ctx, consume_,
        target_type_,
        name_,
        heal_func
    );
}

// 魔法重拳
MagicPunch::MagicPunch() : act(40) {
    name_ = "魔法重拳";
    set_consume("MP", 15);
    set_consume("C_AP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool MagicPunch::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicPunch::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        90,
        DamageFormula::ATK_MATK_COMBINED,
        1.2f,
        name_,
        true
    );
}

// 裂地术
GroundFissure::GroundFissure() : act(70) {
    name_ = "裂地术";
    set_consume("C_AP", 25);
    set_consume("MP", 20);
    set_target_type(TargetType::ALL_ENEMIES);
}

bool GroundFissure::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool GroundFissure::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    return SkillExecutor::execute_aoe_damage(
        c, ctx, consume_,
        target_type_,
        80,
        DamageFormula::MATK,
        1.2f,
        name_,
        true,
        {},
        false
    );
}

// 闪电术
LightningChain::LightningChain() : act(75) {
    name_ = "闪电术";
    set_consume("C_AP", 15);
    set_consume("MP", 20);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
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
        std::cout << textcolor(color::white) << c->get_name() << " 施放闪电术！"
            << resetcolor() << std::endl;
        Sleep(150);
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

    return SkillExecutor::execute_aoe_damage(
        c, ctx, consume_,
        target_type_,
        75,
        DamageFormula::MATK,
        3.0f,
        name_,
        true,
        {},
        false
    );
}

VoidExplosion::VoidExplosion() : act(40) {
    name_ = "虚爆";
    set_consume("MP", 25);
    set_consume("C_AP", 25);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool VoidExplosion::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool VoidExplosion::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        90,
        DamageFormula::ATK_MATK_COMBINED,
        1.8f,
        name_,
        true
    );
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

// 自愈行为
SelfHeal::SelfHeal() : act(20) {
    name_ = "自愈";
    set_consume("C_AP", 15);
    set_consume("MP", 15);
    set_target_type(TargetType::SINGLE_ALLY_SELF);
}

bool SelfHeal::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool SelfHeal::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    int heal_per_turn = static_cast<int>(c->get_attribute("MAX_HP") * 0.1f);
    std::vector<BuffApplication> buffs = {
        {"SELF_HEAL", heal_per_turn, 3}
    };

    return SkillExecutor::execute_self_buff(
        c, ctx, consume_,
        name_,
        buffs
    );
}

VoidGodSummon::VoidGodSummon() : act(100) {
    name_ = "虚神召唤";
    set_consume("MP", 35);
    set_consume("C_AP", 35);
    set_target_type(TargetType::NONE);
}

bool VoidGodSummon::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    if (ctx.summoned == nullptr) return false;
    return ctx.char_team.find(const_cast<character*>(c)) != ctx.char_team.end();
}

bool VoidGodSummon::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    auto it = ctx.char_team.find(c);
    if (it == ctx.char_team.end() || ctx.summoned == nullptr) return false;
    Team* team = const_cast<Team*>(it->second);

    std::string phantom_name = "虚神";
    auto phantom = std::make_unique<character>();
    phantom->set_name(phantom_name);

    int hp = 200;
    int mp = 50;
    int atk = 50;
    int matk = 50;
    int def = 10;
    int spd = 40;
    int crit = 10;
    int crit_d = 300;
    int cap = 50;

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

    phantom->add_buff("SELF_HEAL", 5, 999);
    phantom->add_buff("DEFENSE_BUFF", 50, 999);

    phantom->SetRule(SLOW_BATTLE, c->GetRule(SLOW_BATTLE));
    phantom->SetRule(BATTLE_WITHOUT_OUTPUT, c->GetRule(BATTLE_WITHOUT_OUTPUT));

    phantom->get_actions().clear();
    phantom->get_actions().push_back(std::make_unique<Attack>());
    phantom->get_actions().push_back(std::make_unique<Attack>());
    phantom->get_actions().push_back(std::make_unique<Attack>());
    phantom->get_actions().push_back(std::make_unique<VoidExplosion>());
    phantom->get_actions().push_back(std::make_unique<Smash>());
    phantom->get_actions().push_back(std::make_unique<GroundFissure>());

    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 召唤出 " << phantom_name << "！" << resetcolor() << std::endl;
    }

    ctx.summoned->push_back(std::move(phantom));
    team->add_character(*ctx.summoned->back());

    return true;
}

// 吸血行为
Lifesteal::Lifesteal() : act(20) {
    name_ = "吸血攻击";
    set_consume("C_AP", 20);
    set_consume("MP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
}

bool Lifesteal::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Lifesteal::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    auto on_hit = [](character* caster, character* target, int damage) {
        int steal = damage / 2;
        int new_hp = std::min(caster->get_attribute("MAX_HP"),
            caster->get_attribute("HP") + steal);
        caster->setattribute("HP", new_hp);
        if (!caster->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << customio::textcolor(customio::color::green)
                << caster->get_name() << " 回复了 "
                << customio::textcolor(customio::color::yellow) << steal
                << customio::textcolor(customio::color::green) << " 点生命！"
                << customio::resetcolor() << std::endl;
        }
        };

    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_,
        target_type_,
        100,
        DamageFormula::ATK_MATK_COMBINED,
        1.1f,
        name_,
        true,
        {},
        on_hit
    );
}

// 冥想
Meditate::Meditate() : act(30) {
    name_ = "冥想";
    set_consume("C_AP", 15);
    set_target_type(TargetType::NONE);
}

bool Meditate::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Meditate::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    int mp_restore = static_cast<int>(c->get_attribute("MP") * 0.3f + 10);
    std::unordered_map<std::string, int> attr_mods = {
        {"MP", mp_restore}
    };

    return SkillExecutor::execute_self_buff(
        c, ctx, consume_,
        name_,
        {},
        attr_mods
    );
}

// 防御
Defense::Defense() : act(20) {
    name_ = "防御";
    set_consume("C_AP", 15);
    set_target_type(TargetType::NONE);
}

bool Defense::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Defense::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    std::vector<BuffApplication> buffs = {
        {"DEFENSE_BUFF", 50, 2}
    };

    return SkillExecutor::execute_self_buff(
        c, ctx, consume_,
        name_,
        buffs
    );
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