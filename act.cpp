#include "act.h"
#include "entity.h"          // 需要 character 的完整定义
#include "customio.h"
#include <algorithm>
#include <climits>
#include <math.h>

using namespace customio;
//前置声明
class Crit;

// ---------- 技能注册实现 ----------
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

// 在某个初始化函数中注册所有技能（例如放在 main 或全局对象中）
void registerAllSkills() {
    SkillRegistry::registerSkill("attack", SkillInfo{ "攻击", 100, 10, []() { return std::make_unique<Attack>(); } });
    SkillRegistry::registerSkill("defense", SkillInfo{ "防御", 80, 5, []() { return std::make_unique<Defense>(); } });
    SkillRegistry::registerSkill("smash", SkillInfo{ "冲撞", 30, 8, []() { return std::make_unique<Smash>(); } });
    SkillRegistry::registerSkill("combo", SkillInfo{ "连击", 30, 7, []() { return std::make_unique<ComboAttack>(); } });
    SkillRegistry::registerSkill("magic_missile", SkillInfo{ "魔法飞弹", 50, 9, []() { return std::make_unique<MagicMissile>(); } });
    SkillRegistry::registerSkill("speedup", SkillInfo{ "加速术", 35, 6, []() { return std::make_unique<Speedup>(); } });
    SkillRegistry::registerSkill("poison_magic", SkillInfo{ "毒魔法", 45, 8, []() { return std::make_unique<PoisonMagic>(); } });
    SkillRegistry::registerSkill("fireball", SkillInfo{ "火球术", 55, 9, []() { return std::make_unique<Fireball>(); } });
    SkillRegistry::registerSkill("heal_magic", SkillInfo{ "治愈魔法", 51, 8, []() { return std::make_unique<HealMagic>(); } });
    SkillRegistry::registerSkill("magic_punch", SkillInfo{ "魔法重拳", 52, 8, []() { return std::make_unique<MagicPunch>(); } });
    SkillRegistry::registerSkill("ground_fissure", SkillInfo{ "裂地术", 43, 7, []() { return std::make_unique<GroundFissure>(); } });
    SkillRegistry::registerSkill("lightning_chain", SkillInfo{ "闪电术", 32, 6, []() { return std::make_unique<LightningChain>(); } });
    // 继续添加其他技能...
}

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
Smash::Smash() : act(40) {
    name_ = "冲撞";
    set_consume("C_AP", 23); 
}

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
    int powdamage = std::min((float)std::pow(1.1f, c->get_attribute("ATK")), c->get_attribute("ATK") * 2.5f);    //倍率伤害

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
MagicMissile::MagicMissile() : act(60) {
    name_ = "魔法飞弹";
    set_consume("MP", 20);     
    set_consume("C_AP", 10);
}//暂时先用MP，以后改成C_MP
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
    int atk = c->get_attribute("MATK") * 2.0f; //倍率2.0
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击处理
    int final_damage = CritCalculator::apply(c, base_damage);

    target->take_damage(final_damage);
    return true;
}
//加速术
Speedup::Speedup() : act(40) {
    name_ = "加速术";
    set_consume("MP", 15); 
}

bool Speedup::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    else return true;
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

//毒魔法
PoisonMagic::PoisonMagic() : act(65) {
    name_ = "毒魔法";
    set_consume("MP", 25);
    set_consume("C_AP", 15);
}

bool PoisonMagic::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool PoisonMagic::execute(character* c, FightContext& ctx) {
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
        std::cout << textcolor(color::green) << c->get_name() << " 施放毒魔法！"
            << resetcolor() << std::endl;
    }

    // 命中判定
    const int hit_rate = 85;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 的魔法被躲避了！"
                << resetcolor() << std::endl;
        }
        return true;
    }

    // 基础伤害（魔法攻击 * 1.5倍）
    int atk = c->get_attribute("MATK") * 1.5f;
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 应用暴击
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);

    // 附加毒buff：每回合8点伤害，持续4回合
    target->add_buff("POISON", 8, 4);

    return true;
}

//火球术
Fireball::Fireball() : act(75) {
    name_ = "火球术";
    set_consume("MP", 30);
    set_consume("C_AP", 18);
}

bool Fireball::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool Fireball::execute(character* c, FightContext& ctx) {
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
        std::cout << textcolor(color::red) << c->get_name() << " 施放火球术！"
            << resetcolor() << std::endl;
    }

    // 命中判定
    const int hit_rate = 90;
    if (!chance(hit_rate)) {
        if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::yellow) << c->get_name() << " 的魔法被躲避了！"
                << resetcolor() << std::endl;
        }
        return true;
    }

    // 基础伤害（魔法攻击 * 2.0倍）
    int atk = c->get_attribute("MATK") * 2.0f;
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 应用暴击
    int final_damage = CritCalculator::apply(c, base_damage);
    target->take_damage(final_damage);

    // 附加燃烧buff：每回合12点伤害，持续3回合
    target->add_buff("BURN", 12, 3);

    return true;
}

//治愈魔法
HealMagic::HealMagic() : act(80) {
    name_ = "治愈魔法";
    set_consume("MP", 28);
    set_consume("C_AP", 16);
}

bool HealMagic::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    // 至少有一个友方需要治愈（自己或队友）
    return !ctx.allies.empty();
}

bool HealMagic::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择治愈目标：选择HP生命值最低的友方
    character* target = nullptr;
    int lowest_hp = INT_MAX;
    for (character* ally : ctx.allies) {
        if (!ally || !ally->is_alive()) continue;
        int hp = ally->get_attribute("HP");
        if (hp < lowest_hp) {
            lowest_hp = hp;
            target = ally;
        }
    }

    if (!target) return false;

    // 输出行动信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 施放治愈魔法！"
            << resetcolor() << std::endl;
    }

    // 治愈量：魔法攻击 * 1.8倍 + 50
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

//魔法重拳
MagicPunch::MagicPunch() : act(40) {
    name_ = "魔法重拳";
    set_consume("MP", 12);
    set_consume("C_AP", 12);
}
bool MagicPunch::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();   // 需要有敌人
}

bool MagicPunch::execute(character* c, FightContext& ctx) {
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
        std::cout << textcolor(color::red) << c->get_name() << " 使用魔法重拳!"
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
    int atk = c->get_attribute("MATK") + c->get_attribute("ATK"); //物法双修
    int def = target->get_attribute("DEF");
    int base_damage = std::max(1, atk - def);

    // 暴击处理
    int final_damage = CritCalculator::apply(c, base_damage);

    target->take_damage(final_damage);
    return true;
}

// 裂地术
GroundFissure::GroundFissure() : act(80) {  // 基础成功率80%
    name_ = "裂地术";
    set_consume("C_AP", 20);
    set_consume("MP", 10);
}

bool GroundFissure::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool GroundFissure::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择目标（用于确定队伍）
    character* primary_target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!primary_target) return false;

    // 找到目标所属队伍
    auto it = ctx.char_team.find(primary_target);
    if (it == ctx.char_team.end()) return false;
    const Team* target_team = it->second;

    // 收集该队伍的所有存活敌人
    std::vector<character*> team_enemies;
    for (character* enemy : ctx.enemies) {
        auto team_it = ctx.char_team.find(enemy);
        if (team_it != ctx.char_team.end() && team_it->second == target_team) {
            team_enemies.push_back(enemy);
        }
    }
    if (team_enemies.empty()) return false;

    int num_targets = team_enemies.size();
    // 伤害基数 = MATK * 2.0
    int base_damage = static_cast<int>(c->get_attribute("MATK") * 2.0f);
    // 总伤害 = 基数 * (1 + 人数 * 0.15) ，人数越多总伤害越高
    float total_multiplier = 1.0f + num_targets * 0.15f;
    int total_damage = static_cast<int>(base_damage * total_multiplier);
    // 每个敌人受到的伤害 = 总伤害 / 人数（向下取整，但至少1点）
    int per_target_damage = std::max(1, total_damage / num_targets);

    // 输出技能信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << c->get_name() << " 施放裂地术！"
            << resetcolor() << std::endl;
    }

    // 对每个目标造成伤害
    for (character* target : team_enemies) {
        // 命中判定（每个目标独立）
        const int hit_rate = 85;
        if (!chance(hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << " 攻击 " << target->get_name() << " 落空了！"
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
}

bool LightningChain::can_execute(const character* c, const FightContext& ctx) const {
    if (!act::can_execute(c, ctx)) return false;
    return !ctx.enemies.empty();
}

bool LightningChain::execute(character* c, FightContext& ctx) {
    if (!can_execute(c, ctx)) return false;

    // 扣除消耗
    for (const auto& pair : consume_) {
        int old = c->get_attribute(pair.first);
        c->setattribute(pair.first, old - pair.second, false);
    }

    // 选择初始目标
    character* current_target = TargetSelector::select_lowest_hp(ctx.enemies);
    if (!current_target) return false;

    // 输出技能信息
    if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::cyan) << c->get_name() << " 施放闪电术！"
            << resetcolor() << std::endl;
    }

    // 基础伤害 = MATK * 2.0
    int base_damage = static_cast<int>(c->get_attribute("MATK") * 2.0f);
    const float decay_factor = 0.7f;   // 每次传导衰减30%
    const int max_jumps = 3;            // 最多额外传导3次（共4个目标）
    std::vector<character*> hit_targets;
    hit_targets.push_back(current_target);

    // 传导逻辑
    for (int jump = 0; jump < max_jumps; ++jump) {
        // 找出尚未被击中的敌人
        std::vector<character*> available;
        for (character* enemy : ctx.enemies) {
            if (std::find(hit_targets.begin(), hit_targets.end(), enemy) == hit_targets.end()) {
                available.push_back(enemy);
            }
        }
        if (available.empty()) break;

        // 随机选择一个新目标
        static std::mt19937& rng = customio::get_random_engine();
        std::uniform_int_distribution<int> dist(0, available.size() - 1);
        character* next_target = available[dist(rng)];
        hit_targets.push_back(next_target);
    }

    // 依次造成伤害（伤害递减）
    for (size_t idx = 0; idx < hit_targets.size(); ++idx) {
        character* target = hit_targets[idx];
        float multiplier = std::pow(decay_factor, idx);  // 第0次无衰减
        int damage = static_cast<int>(base_damage * multiplier);
        if (damage < 1) damage = 1;

        // 命中判定
        const int hit_rate = 85;
        if (!chance(hit_rate)) {
            if (!c->GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << " 闪电击中 " << target->get_name() << " 但被躲开了！"
                    << resetcolor() << std::endl;
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
Defense::Defense() : act(20) {
    name_ = "防御";
    set_consume("C_AP", 20);
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

    // 使用buff系统添加防御增益效果（持续1回合）
    // 效果值50代表减伤50%
    const_cast<character*>(c)->add_buff("DEFENSE_BUFF", 50, 1);

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