// act.cpp
#include "act.h"
#include "entity.h"
#include "customio.h"
#include "skill_executor.h"
#include "damage_calculator.h"
#include "skill_data.h"
#include "generic_skill.h"
#include "skill_loader.h"
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

void SkillRegistry::clearAll() {
    skills_.clear();
}

const std::unordered_map<std::string, SkillInfo>& SkillRegistry::getAllSkills() {
    return skills_;
}

void registerAllSkills() {
    // 从外部 JSON 加载纯数据驱动技能
    auto external_skills = SkillLoader::load_from_directory("config/skills/");
    for (const auto& data : external_skills) {
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

    // 注册保留的派生类技能（无法通过 JSON 配置的）
    SkillRegistry::registerSkill("attack", SkillInfo("攻击", 100, 10, []() { return std::make_unique<Attack>(); }));
    SkillRegistry::registerSkill("defense", SkillInfo("防御", 80, 5, []() { return std::make_unique<Defense>(); }));
    SkillRegistry::registerSkill("smash", SkillInfo("冲撞", 40, 8, []() { return std::make_unique<Smash>(); }));
    SkillRegistry::registerSkill("combo", SkillInfo("连击", 30, 7, []() { return std::make_unique<ComboAttack>(); }));
    SkillRegistry::registerSkill("magic_missile", SkillInfo("魔法飞弹", 50, 9, []() { return std::make_unique<MagicMissile>(); }));
    SkillRegistry::registerSkill("speedup", SkillInfo("加速术", 35, 6, []() { return std::make_unique<Speedup>(); }));
    SkillRegistry::registerSkill("poison_magic", SkillInfo("毒魔法", 45, 8, []() { return std::make_unique<PoisonMagic>(); }));
    SkillRegistry::registerSkill("fireball", SkillInfo("火球术", 55, 9, []() { return std::make_unique<Fireball>(); }));
    SkillRegistry::registerSkill("freeze", SkillInfo("冰冻术", 40, 8, []() { return std::make_unique<Freeze>(); }));
    SkillRegistry::registerSkill("heal_magic", SkillInfo("治愈魔法", 50, 9, []() { return std::make_unique<HealMagic>(); }));
    SkillRegistry::registerSkill("summon_phantom", SkillInfo("召唤幻影", 25, 9, []() { return std::make_unique<SummonPhantom>(); }));
    SkillRegistry::registerSkill("lightning", SkillInfo("闪电术", 20, 8, []() { return std::make_unique<LightningChain>(); }));
    SkillRegistry::registerSkill("self_heal", SkillInfo("自愈", 7, 10, []() { return std::make_unique<SelfHeal>(); }));
    SkillRegistry::registerSkill("void_god_summon", SkillInfo("虚神召唤", 10, 10, []() { return std::make_unique<VoidGodSummon>(); }));
    SkillRegistry::registerSkill("lifesteal", SkillInfo("吸血", 20, 8, []() { return std::make_unique<Lifesteal>(); }));
    SkillRegistry::registerSkill("meditate", SkillInfo("冥想", 25, 7, []() { return std::make_unique<Meditate>(); }));
    SkillRegistry::registerSkill("magic_punch", SkillInfo("魔法重拳", 60, 10, []() { return std::make_unique<MagicPunch>(); }));
    SkillRegistry::registerSkill("void_explosion", SkillInfo("虚爆", 7, 10, []() { return std::make_unique<VoidExplosion>(); }));
    SkillRegistry::registerSkill("ground_fissure", SkillInfo("裂地术", 30, 7, []() { return std::make_unique<GroundFissure>(); }));
    SkillRegistry::registerSkill("meteor_strike", SkillInfo("陨石打击", 7, 10, []() { return std::make_unique<MeteorStrike>(); }));
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
    set_description("朴实无华的平A。");
}

bool Attack::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Attack::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        90, DamageFormula::ATK, 1.0f, name_, true, {}, nullptr);
}

// 冲撞
Smash::Smash() : act(40) {
    name_ = "冲撞";
    set_consume("C_AP", 23);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("七星级的动能定理！");
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
        customio::game_sleep(300 * g_battle_speed);
    }
    //概率击晕
        if (SkillExecutor::hit_check(c, 85)) {
            target->add_buff("STUNNED", 1, 1); // 眩晕1回合
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                const auto& theme = customio::get_console_theme();
                std::cout << customio::adaptive_textcolor(theme.special)
                        << target->get_name() << " 被击晕了！"
                        << customio::resetcolor() << std::endl;
            }
        }
    int raw_atk = DamageCalculator::calculate_raw_attack(c, DamageFormula::ATK_POWER);
    int final_damage = DamageCalculator::calculate_final_damage(c, target, raw_atk, true);
    target->take_damage(final_damage);
    return true;
}

// 连击
ComboAttack::ComboAttack() : act(70) {
    name_ = "连击";
    set_consume("C_AP", 20);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("一秒三刀，刀刀暴击？不存在的，但痛是真的痛。");
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
    const int max_hits = 5;
    const float damage_coeff = 0.95f;
    const float decay = 0.92f;
    const int hit_rate = 93;
    for (int i = 0; i < max_hits; ++i) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << customio::textcolor(customio::color::white)
                      << c->get_name() << " 连击第" << i + 1 << "段！"
                      << customio::resetcolor() << std::endl;
            customio::game_sleep(200 * g_battle_speed);
        }
        if (!SkillExecutor::hit_check(c, hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << customio::textcolor(customio::color::yellow)
                          << "连击未命中！" << customio::resetcolor() << std::endl;
            }
        } else {
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
    set_description("biu~ biu~ biu~ 魔法飞弹，谁用谁知道。");
}

bool MagicMissile::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicMissile::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        90, DamageFormula::MATK, 1.6f, name_, true);
}

// 召唤幻影
SummonPhantom::SummonPhantom() : act(35) {
    name_ = "召唤幻影";
    set_consume("MP", 12);
    set_consume("C_AP", 12);
    set_target_type(TargetType::NONE);
    set_description("分身术！虽然分身有点脆，但人多势众。");
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
    int hp = std::max(1, c->get_attribute("HP") * 100 / 100);
    int mp = std::max(0, c->get_attribute("MP") * 70 / 100);
    int atk = std::max(1, c->get_attribute("ATK") * 90 / 100);
    int matk = std::max(1, c->get_attribute("MATK") * 90 / 100);
    int def = std::max(1, c->get_attribute("DEF") * 80 / 100);
    int spd = std::max(1, c->get_attribute("SPD") * 90 / 100);
    int crit = c->get_attribute("CRIT");
    int crit_d = c->get_attribute("CRIT_D");
    int cap = std::max(30, c->get_attribute("C_AP"));
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
        customio::game_sleep(300 * g_battle_speed);
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
    set_description("风魔法的分支，给自己脚下抹油，跑得更快，动手更频。");
}

bool Speedup::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Speedup::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    std::unordered_map<std::string, int> attr_mods = { {"C_AP", 25} };
    return SkillExecutor::execute_self_buff(c, ctx, consume_, name_, {}, attr_mods);
}

// 毒魔法
PoisonMagic::PoisonMagic() : act(65) {
    name_ = "毒魔法";
    set_consume("MP", 25);
    set_consume("C_AP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("绿色环保，无毒无害（才怪），让敌人中毒掉血。");
}

bool PoisonMagic::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool PoisonMagic::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    int poison_damage = static_cast<int>(c->get_attribute("MATK") * 0.2f);
    std::vector<BuffApplication> buffs = { {"POISON", poison_damage, 3} };
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        85, DamageFormula::MATK, 1.5f, name_, true, buffs);
}

// 火球术
Fireball::Fireball() : act(75) {
    name_ = "火球术";
    set_consume("MP", 30);
    set_consume("C_AP", 18);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("火遁·豪火球之术！");
}

bool Fireball::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Fireball::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    int burn_damage = static_cast<int>(c->get_attribute("MATK") * 0.3f);
    std::vector<BuffApplication> buffs = { {"BURN", burn_damage, 2} };
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        90, DamageFormula::MATK, 2.0f, name_, true, buffs);
}

// 治愈魔法
HealMagic::HealMagic() : act(80) {
    name_ = "治愈魔法";
    set_consume("MP", 28);
    set_consume("C_AP", 16);
    set_target_type(TargetType::SINGLE_ALLY_LOWEST_HP);
    set_description("奶妈的爱，一奶就满。");
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
    return SkillExecutor::execute_heal(c, ctx, consume_, target_type_, name_, heal_func);
}

// 魔法重拳
MagicPunch::MagicPunch() : act(40) {
    name_ = "魔法重拳";
    set_consume("MP", 15);
    set_consume("C_AP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("把魔力灌进拳头，一拳下去敌人会喊“这是什么鬼伤害？”");
}

bool MagicPunch::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MagicPunch::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        90, DamageFormula::ATK_MATK_COMBINED, 1.2f, name_, true);
}

// 裂地术
GroundFissure::GroundFissure() : act(70) {
    name_ = "裂地术";
    set_consume("C_AP", 25);
    set_consume("MP", 20);
    set_target_type(TargetType::ALL_ENEMIES);
    set_description("大地母亲在忽悠着你……哦不，是大地在震动！");
}

bool GroundFissure::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool GroundFissure::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_aoe_damage(
        c, ctx, consume_, target_type_,
        80, DamageFormula::MATK, 1.2f, name_, true, {}, false);
}

// 闪电术
LightningChain::LightningChain() : act(75) {
    name_ = "闪电术";
    set_consume("C_AP", 15);
    set_consume("MP", 20);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("链状闪电，让你体验被电击的酸爽。");
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
        std::cout << textcolor(color::white) << c->get_name() << " 施放闪电术！" << resetcolor() << std::endl;
        customio::game_sleep(300 * g_battle_speed);
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
        if (!chance(85)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT))
                std::cout << textcolor(color::yellow) << " 闪电击中 " << target->get_name() << " 但被躲开了！" << resetcolor() << std::endl;
            continue;
        }
        int final_damage = CritCalculator::apply(c, damage);
        target->take_damage(final_damage);
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT))
            std::cout << textcolor(color::yellow) << " 闪电传导至 " << target->get_name() << "，造成 " << final_damage << " 点伤害！" << resetcolor() << std::endl;
    }
    return true;
}

MeteorStrike::MeteorStrike() : act(50) {
    name_ = "陨石打击";
    set_consume("MP", 40);
    set_consume("C_AP", 30);
    set_target_type(TargetType::ALL_ENEMIES);
    set_description("呼叫轨道炮，对地轰炸。");
}

bool MeteorStrike::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool MeteorStrike::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_aoe_damage(
        c, ctx, consume_, target_type_,
        75, DamageFormula::MATK, 3.0f, name_, true, {}, false);
}

VoidExplosion::VoidExplosion() : act(40) {
    name_ = "虚爆";
    set_consume("MP", 25);
    set_consume("C_AP", 25);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("虚空能量压缩后爆发，据说会炸出黑洞。(用脚填数值的典范)");
}

bool VoidExplosion::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool VoidExplosion::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        90, DamageFormula::ATK_MATK_COMBINED, 1.8f, name_, true);
}

// 暴击判定
int CritCalculator::apply(character* attacker, int base_damage) {
    if (!attacker) return base_damage;
    int crit_rate = attacker->get_attribute("CRIT");
    if (customio::chance(crit_rate)) {
        int crit_dmg = attacker->get_attribute("CRIT_D");
        int final_damage = base_damage * crit_dmg / 100;
        if (!attacker->GetRule(BATTLE_WITHOUT_OUTPUT))
            std::cout << customio::textcolor(customio::color::red) << "★ 暴击！" << customio::resetcolor() << std::endl;
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
    set_description("反转术式（？）");
}

bool SelfHeal::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool SelfHeal::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    int heal_per_turn = static_cast<int>(c->get_attribute("MAX_HP") * 0.1f);
    std::vector<BuffApplication> buffs = { {"SELF_HEAL", heal_per_turn, 3} };
    return SkillExecutor::execute_self_buff(c, ctx, consume_, name_, buffs);
}

VoidGodSummon::VoidGodSummon() : act(100) {
    name_ = "虚神召唤";
    set_consume("MP", 50);
    set_consume("C_AP", 35);
    set_consume("HP", 50);
    set_target_type(TargetType::NONE);
    set_description("我的天哪，魔虚罗大人。");
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
    int hp = 300, mp = 50, atk = 45, matk = 45, def = 10, spd = 60, crit = 20, crit_d = 300, cap = 30;
    phantom->setattribute("HP", hp); phantom->setattribute("MAX_HP", hp);
    phantom->setattribute("MP", mp); phantom->setattribute("MAX_MP", mp);
    phantom->setattribute("ATK", atk); phantom->setattribute("MATK", matk);
    phantom->setattribute("DEF", def); phantom->setattribute("SPD", spd);
    phantom->setattribute("CRIT", crit); phantom->setattribute("CRIT_D", crit_d);
    phantom->setattribute("C_AP", cap);
    phantom->SetRule(SLOW_BATTLE, c->GetRule(SLOW_BATTLE));
    phantom->SetRule(BATTLE_WITHOUT_OUTPUT, c->GetRule(BATTLE_WITHOUT_OUTPUT));
    phantom->get_actions().clear();
    phantom->get_actions().push_back(std::make_unique<HealMagic>());
    phantom->get_actions().push_back(std::make_unique<VoidExplosion>());
    phantom->get_actions().push_back(std::make_unique<Smash>());
    phantom->get_actions().push_back(std::make_unique<GroundFissure>());
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT))
        std::cout << textcolor(color::magenta) << c->get_name() << " 召唤出 " << phantom_name << "！" << resetcolor() << std::endl;
    ctx.summoned->push_back(std::move(phantom));
    team->add_character(*ctx.summoned->back());
    return true;
}

Freeze::Freeze() : act(30) {
    name_ = "冰冻术";
    set_consume("MP", 18);
    set_consume("C_AP", 12);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("冰魔法的分支，让敌人冻结无法行动。");
}

bool Freeze::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Freeze::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    std::vector<BuffApplication> buffs = { {"FREEZE", 0, 2} };
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        70, DamageFormula::MATK, 1.0f, name_, true, buffs);
}

// 吸血行为
Lifesteal::Lifesteal() : act(20) {
    name_ = "吸血攻击";
    set_consume("C_AP", 20);
    set_consume("MP", 15);
    set_target_type(TargetType::SINGLE_ENEMY_LOWEST_HP);
    set_description("吸血鬼附体，你的血是我的药。");
}

bool Lifesteal::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Lifesteal::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    auto on_hit = [](character* caster, character* target, int damage) {
        int steal = damage / 2;
        int new_hp = std::min(caster->get_attribute("MAX_HP"), caster->get_attribute("HP") + steal);
        caster->setattribute("HP", new_hp);
        if (!caster->GetRule(BATTLE_WITHOUT_OUTPUT))
            std::cout << customio::textcolor(customio::color::green) << caster->get_name() << " 回复了 "
                      << customio::textcolor(customio::color::yellow) << steal
                      << customio::textcolor(customio::color::green) << " 点生命！" << customio::resetcolor() << std::endl;
    };
    return SkillExecutor::execute_single_target_damage(
        c, ctx, consume_, target_type_,
        100, DamageFormula::ATK_MATK_COMBINED, 1.1f, name_, true, {}, on_hit);
}

// 冥想
Meditate::Meditate() : act(30) {
    name_ = "冥想";
    set_consume("C_AP", 15);
    set_target_type(TargetType::NONE);
    set_description("醉生梦死？不对，我在等大招，你在等什么？");
}

bool Meditate::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Meditate::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    int mp_restore = static_cast<int>(c->get_attribute("MP") * 0.3f + 10);
    std::unordered_map<std::string, int> attr_mods = { {"MP", mp_restore} };
    return SkillExecutor::execute_self_buff(c, ctx, consume_, name_, {}, attr_mods);
}

// 防御
Defense::Defense() : act(20) {
    name_ = "防御";
    set_consume("C_AP", 15);
    set_target_type(TargetType::NONE);
    set_description("抱头蹲防，伤害减半。");
}

bool Defense::can_execute(const character* c, const FightContext& ctx) const {
    return act::can_execute(c, ctx);
}

bool Defense::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;
    std::vector<BuffApplication> buffs = { {"DEFENSE_BUFF", 50, 2} };
    return SkillExecutor::execute_self_buff(c, ctx, consume_, name_, buffs);
}

// ---------- 目标选择器实现 ----------
character* TargetSelector::select_lowest_hp(const std::vector<character*>& enemies) {
    character* target = nullptr;
    int lowest_hp = INT_MAX;
    for (character* e : enemies) {
        if (!e || !e->is_alive()) continue;
        int hp = e->get_attribute("HP");
        if (hp < lowest_hp) { lowest_hp = hp; target = e; }
    }
    return target;
}