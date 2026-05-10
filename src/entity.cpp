// entity.cpp
#include "entity.h"
#include "customio.h"
#include "console.h"
#include "act.h"
#include "weapon_data.h"
#include <iomanip>
#include <random>
#include <chrono>
#include <climits>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace customio;

// -------------------- character 实现 --------------------

std::mt19937& character::get_random_engine() {
    static std::mt19937 engine(std::chrono::system_clock::now().time_since_epoch().count());
    return engine;
}

character::character() {
    SetRule(SUMMON_BASIC_ATTR, false);
    SetRule(SHOW_ATTRIBUTES, true);
    init_default_actions();
}

std::unique_ptr<character> character::clone_for_battle() const {
    auto clone = std::make_unique<character>();
    
    // ---- 1. 基础信息 ----
    clone->name_ = name_;
    clone->hashcode = hashcode;        // 保留哈希，虽然不会用于生成属性
    
    // 关键：禁用随机属性生成与随机技能生成
    clone->SetRule(SUMMON_BASIC_ATTR, false);
    clone->SetRule(SHOW_ATTRIBUTES, false);
    
    // ---- 2. 深拷贝属性表 ----
    for (const auto& [key, pair] : attribute_) {
        clone->attribute_[key] = pair;
    }
    // 强制满状态
    clone->setattribute("HP", clone->get_attribute("MAX_HP"));
    clone->setattribute("MP", clone->get_attribute("MAX_MP"));
    // 确保行动点也正确（不重置为0）
    // 已经通过循环拷贝，无需额外处理
    
    // ---- 3. 复制规则标志 ----
    clone->rule_ = rule_;   // 直接复制整个规则表
    
    // ---- 4. 复制加护（可选，但一般不需要在克隆体中体现） ----
    clone->aegis = aegis;
    
    // ---- 5. 深拷贝技能列表 ----
    clone->actions_.clear();
    for (const auto& action : actions_) {
        const std::string& skill_name = action->get_name();
        const SkillInfo* info = SkillRegistry::getSkillInfo(skill_name);
        if (info != nullptr) {
            clone->actions_.push_back(info->factory());
        } else {
            // 技能未注册，添加普攻作为后备
            clone->actions_.push_back(std::make_unique<Attack>());
        }
    }
    // 如果完全没有技能（不太可能），添加普攻
    if (clone->actions_.empty()) {
        clone->actions_.push_back(std::make_unique<Attack>());
    }
    
    // ---- 6. 复制 Buff 状态（重要！否则永久 Buff 会丢失） ----
    clone->buffs_ = buffs_;
    
    // ---- 7. 重置战斗统计数据 ----
    clone->damage_dealt = 0;
    clone->damage_taken = 0;
    clone->kills = 0;
    clone->healing_done = 0;
    
    return clone;
}

void character::attack(entity& target) {
    // 旧版攻击逻辑已弃用，现在由行为系统处理
}

character::character(std::string name) : name_(name) {
    // ---------- 导入角色 ([:id] 格式) ----------
    if (name.length() > 4 && name[0] == '[' && name[1] == ':' && name[name.length() - 1] == ']') {
        std::string char_id = name.substr(2, name.length() - 3);
        const ImportedCharacterData* data = get_imported_character(char_id);
        if (data) {
            name_ = data->name;
            SetRule(SUMMON_BASIC_ATTR, false);
            SetRule(SHOW_ATTRIBUTES, true);

            // 1. 应用 JSON 中明确定义的属性
            for (const auto& pair : data->attributes) {
                setattribute(pair.first, pair.second);
            }

            // 2. 补齐所有缺失的核心属性（保证战斗系统稳定）
            const std::unordered_map<std::string, int> default_attrs = {
                {"MAX_HP", 100}, {"HP",  100},
                {"MAX_MP",  50}, {"MP",   50},
                {"ATK",     30}, {"DEF",  10},
                {"MATK",    30}, {"SPD",  20},
                {"CRIT",    10}, {"CRIT_D", 150},
                {"C_AP",    20}
            };

            for (const auto& [attr, val] : default_attrs) {
                // 仅当属性不存在或值为 0 时才填入默认值
                if (attribute_.find(attr) == attribute_.end() || get_attribute(attr) == 0) {
                    setattribute(attr, val);
                }
            }

            // 特殊保证：HP 和 MP 不能为 0
            if (get_attribute("HP") == 0)
                setattribute("HP", get_attribute("MAX_HP"));
            if (get_attribute("MP") == 0)
                setattribute("MP", get_attribute("MAX_MP"));

            // 3. 导入技能（若 JSON 未提供则随机生成）
            if (!data->actions.empty()) {
                actions_.clear();
                for (const auto& action_name : data->actions) {
                    std::string lower_name = action_name;
                    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    const SkillInfo* info = SkillRegistry::getSkillInfo(lower_name);
                    if (info) {
                        actions_.push_back(info->factory());
                    } else {
                        std::cout << customio::textcolor(customio::color::yellow)
                                  << "警告：未知行为 '" << action_name << "' 被跳过"
                                  << customio::resetcolor() << std::endl;
                    }
                }
            } else {
                init_default_actions();
            }

            // 4. 武器状态初始化
            equipped_weapon_id_.clear();
            weapon_bonus_.clear();
            weapon_granted_action_ = nullptr;

            return;   // 导入完成，直接返回
        }
        // 若未找到导入数据，会继续向下执行，按普通角色创建（但此时 name 仍是原始[:id]格式）
    }

    // ---------- 普通随机角色 ----------
    SetRule(SUMMON_BASIC_ATTR, true);
    SetRule(SHOW_ATTRIBUTES, true);
    setbasicattr();            // 根据名字哈希生成属性与加护
    init_default_actions();    // 按权重随机学习技能
}

void character::setaegis() {
    // ========== 万分之一超级加护（保留原有三种） ==========
    if (hashcode % 10000 == 0) {
        aegis.push_back("Sovereign"); // 主宰
        setattribute("MATK", 0);
        setattribute("MP", 0);
        setattribute("ATK", get_attribute("ATK") + 20);
        setattribute("CRIT", get_attribute("CRIT") + 50);
        setattribute("CRIT_D", get_attribute("CRIT_D") - 50);
        setattribute("DEF", get_attribute("DEF") + 20);
        setattribute("HP", std::min(get_attribute("HP") + 40, 220));
        setattribute("MAX_HP", get_attribute("MAX_HP") + 40);
        setattribute("SPD", std::min(get_attribute("SPD") * 4, 80));
        setattribute("C_AP", std::min(get_attribute("C_AP") + 60, 80));
        buffs_["DEFENSE_BUFF"] = { 50, 999 };
        return;
    }
    else if (hashcode % 10000 == 1) {
        aegis.push_back("Guardian"); // 守护者
        setattribute("MATK", get_attribute("MATK") - 5);
        setattribute("ATK", get_attribute("ATK") - 5);
        setattribute("DEF", get_attribute("DEF") + 30);
        setattribute("HP", 250);
        setattribute("MAX_HP", 250);
        buffs_["DEFENSE_BUFF"] = { 93, 999 };
        actions_.push_back(std::make_unique<SelfHeal>()); // 额外自带一个强力自愈
        return;
    }
    else if (hashcode % 10000 == 2) {
        aegis.push_back("Aeternus"); // 永恒者
        setattribute("MATK", 55);
        setattribute("MP", 999);
        setattribute("SPD", std::min(get_attribute("SPD") * 2, 60));
        setattribute("C_AP", std::min(get_attribute("C_AP") + 40, 60));
        buffs_["DEFENSE_BUFF"] = { 50, 999 };
        buffs_["SELF_HEAL"] = { 20, 999 };   //自愈buff
        return;
    }

    // ========== 10% 普通加护 ==========
    int roll = std::abs(hashcode) % 100;
    if (roll >= 10) return;   // 90% 无加护

    // 所有普通加护都有一小部分通用提升
    setattribute("MAX_HP", get_attribute("MAX_HP") + 20);
    setattribute("HP", get_attribute("MAX_HP"));
    setattribute("DEF", get_attribute("DEF") + 3);

    switch (roll) {
        case 0: {
            aegis.push_back("Warrior"); // 战士
            setattribute("ATK", get_attribute("ATK") + 15);
            setattribute("CRIT", get_attribute("CRIT") + 10);
            actions_.push_back(std::make_unique<Smash>()); // 额外自带一个撞击
            break;
        }
        case 1: {
            aegis.push_back("Mage"); // 法师
            setattribute("MATK", get_attribute("MATK") + 15);
            setattribute("MP", get_attribute("MP") + 30);
            actions_.push_back(std::make_unique<Meditate>()); // 额外自带一个冥想
            break;
        }
        case 2: {
            aegis.push_back("Guard"); // 守卫
            setattribute("DEF", get_attribute("DEF") + 12);
            setattribute("HP", get_attribute("HP") + 30);
            setattribute("MAX_HP", get_attribute("MAX_HP") + 30);
            break;
        }
        case 3: {
            aegis.push_back("Assassin"); // 刺客
            setattribute("SPD", get_attribute("SPD") + 12);
            setattribute("CRIT", get_attribute("CRIT") + 15);
            setattribute("ATK", get_attribute("ATK") + 8);
            break;
        }
        case 4: {
            aegis.push_back("Priest"); // 牧师
            setattribute("MATK", get_attribute("MATK") + 10);
            setattribute("MP", get_attribute("MP") + 40);
            buffs_["SELF_HEAL"] = { 10, 999 }; // 每回合回复10点
            break;
        }
        case 5: {
            aegis.push_back("Berserker"); // 狂战士
            setattribute("ATK", get_attribute("ATK") + 20);
            setattribute("DEF", get_attribute("DEF") - 5);
            setattribute("CRIT", get_attribute("CRIT") + 10);
            break;
        }
        case 6: {
            aegis.push_back("Sage"); // 贤者
            setattribute("MATK", get_attribute("MATK") + 12);
            setattribute("MP", get_attribute("MP") + 50);
            setattribute("SPD", get_attribute("SPD") + 8);
            break;
        }
        case 7: {
            aegis.push_back("Knight"); // 骑士
            setattribute("ATK", get_attribute("ATK") + 10);
            setattribute("DEF", get_attribute("DEF") + 8);
            setattribute("HP", get_attribute("HP") + 20);
            setattribute("MAX_HP", get_attribute("MAX_HP") + 20);
            break;
        }
        case 8: {
            aegis.push_back("Scout"); // 侦察兵
            setattribute("SPD", get_attribute("SPD") + 15);
            setattribute("C_AP", get_attribute("C_AP") + 15);
            setattribute("CRIT", get_attribute("CRIT") + 5);
            break;
        }
        case 9: {
            aegis.push_back("Druid"); // 德鲁伊
            setattribute("ATK", get_attribute("ATK") + 8);
            setattribute("MATK", get_attribute("MATK") + 8);
            setattribute("HP", get_attribute("HP") + 15);
            setattribute("MAX_HP", get_attribute("MAX_HP") + 15);
            break;
        }
    }
}

void character::init_default_actions() {
    actions_.clear();
    if (hashcode == 0 && name_.empty()) {
        actions_.push_back(std::make_unique<Attack>());
        actions_.push_back(std::make_unique<Defense>());
        return;
    }
    const auto& all_skills = SkillRegistry::getAllSkills();
    if (all_skills.empty()) {
        actions_.push_back(std::make_unique<Attack>());
        actions_.push_back(std::make_unique<Defense>());
        return;
    }
    std::vector<std::pair<std::string, SkillInfo>> sorted_skills(all_skills.begin(), all_skills.end());
    std::sort(sorted_skills.begin(), sorted_skills.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    uint64_t role_seed = std::hash<std::string>{}(name_);
    std::mt19937 rng(static_cast<unsigned>(role_seed));
    for (const auto& pair : sorted_skills) {
        const SkillInfo& info = pair.second;
        if (!info.random_available) continue; // 仅习得但不随机选中
        std::uniform_int_distribution<int> dist(0, 99);
        int r = dist(rng);
        if (r < info.acquire_chance) {
            actions_.push_back(info.factory());
        }
    }
    if (actions_.empty()) {
        actions_.push_back(std::make_unique<Attack>());
    }
}

bool character::do_action(FightContext& ctx) {
    //如果被眩晕或者冰冻了，就不能行动
    auto debuff_it = buffs_.find("STUNNED");
    if (debuff_it != buffs_.end() && debuff_it->second.second > 0) {
        if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
            const auto& theme = get_console_theme();
            std::cout << adaptive_textcolor(theme.special) << name_ << " 被眩晕了，无法行动！" << resetcolor() << std::endl;
        }
        return false;
    }
     debuff_it = buffs_.find("FROZEN");
    if (debuff_it != buffs_.end() && debuff_it->second.second > 0) {
        if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
            const auto& theme = get_console_theme();
            std::cout << adaptive_textcolor(theme.special) << name_ << " 被冰冻了，无法行动！" << resetcolor() << std::endl;
        }
        return false;
    }
    std::vector<std::pair<act*, int>> available;
    int total_weight = 0;
    for (auto& action : actions_) {
        if (action->can_execute(this, ctx)) {
            const SkillInfo* info = SkillRegistry::getSkillInfo(action->get_name());
            int weight = info ? info->weight : 1;
            available.emplace_back(action.get(), weight);
            total_weight += weight;
        }
    }
    if (available.empty()) return false;
    static std::mt19937& rng = customio::get_random_engine();
    std::uniform_int_distribution<int> dist(1, total_weight);
    int roll = dist(rng);
    int accum = 0;
    for (auto& pair : available) {
        accum += pair.second;
        if (roll <= accum) {
            pair.first->execute(this, ctx);
            return true;
        }
    }
    return false;
}

void character::setattribute(const std::string& attr_name, int value, bool show) {
    attribute_[attr_name] = { value, show };
}

void character::setattribute(const std::string& attr_name, int value) {
    attribute_[attr_name] = { value, true };
}

int character::get_attribute(const std::string& attr_name) const {
    auto it = attribute_.find(attr_name);
    return it != attribute_.end() ? it->second.first : 0;
}

std::string character::get_name() const {
    return name_;
}

void character::set_name(std::string name) {
    name_ = name;
}

bool character::is_alive() const {
    return get_attribute("HP") > 0;
}

void character::take_damage(int damage) {
    take_damage(damage, nullptr);
}

void character::take_damage(int damage, character* attacker) {
    if (damage <= 0) return;

    // 防御 Buff 处理（先计算减免后的伤害）
    int def_buff = 0;
    auto buff_it = buffs_.find("DEFENSE_BUFF");
    if (buff_it != buffs_.end()) {
        def_buff = buff_it->second.first;
    }
    if (def_buff == 0) {
        def_buff = get_attribute("DEFENSE_BUFF");
    }
    if (def_buff > 0) {
        damage = damage * (100 - def_buff) / 100;
        if (damage <= 0) damage = 1;
        if (get_attribute("DEFENSE_BUFF") > 0) {
            setattribute("DEFENSE_BUFF", 0, false);
        }
    }

    // 统计：使用最终减免后的伤害
    damage_taken += damage;
    if (attacker) {
        attacker->damage_dealt += damage;
    }

    // 扣血
    int current_hp = get_attribute("HP");
    int new_hp = std::max(0, current_hp - damage);
    setattribute("HP", new_hp);

    // 日志输出（完整恢复）
    if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
        const auto& theme = get_console_theme();
        std::cout << adaptive_textcolor(theme.text) << name_ << " 受到了 "
            << adaptive_textcolor(theme.damage) << damage
            << adaptive_textcolor(theme.text) << " 点伤害，剩余HP：" << get_attribute("HP")
            << resetcolor() << std::endl;
    }

    // 死亡处理
    if (get_attribute("HP") == 0) {
        if (attacker) {
            attacker->kills++;   // 攻击者击杀数+1
        }
        if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
            const auto& theme = get_console_theme();
            std::cout << adaptive_textcolor(theme.special) << bold() << name_
                << " 被击败了！" << resetcolor() << std::endl;
        }
    }
}

int character::calculate_damage(character& target) {
    auto& rng = get_random_engine();
    std::uniform_int_distribution<int> hit_dist(1, 100);
    int hit_chance = hit_dist(rng);
    if (hit_chance > 90) return 0;
    int atk = get_attribute("ATK");
    int def = target.get_attribute("DEF");
    int base_dmg = std::max(1, atk - def);
    int crit_rate = get_attribute("CRIT");
    int crit_dmg = get_attribute("CRIT_D");
    std::uniform_int_distribution<int> crit_dist(1, 100);
    int crit_chance = crit_dist(rng);
    if (crit_chance <= crit_rate) {
        return base_dmg * crit_dmg / 100;
    }
    return base_dmg;
}

void character::setbasicattr() {
    if (rule_[SUMMON_BASIC_ATTR]) {
        hashcode = std::hash<std::string>{}(name_);
        uint64_t uh = std::abs(hashcode);
        setattribute("HP", 120 + (uh % 60), true);
        setattribute("MAX_HP", get_attribute("HP"), false);
        setattribute("MP", 20 + ((uh >> 7) % 80), true);
        setattribute("ATK", 30 + ((uh >> 1) % 20), true);
        setattribute("MATK", 30 + ((uh >> 6) % 20), true);
        setattribute("DEF", 10 + ((uh >> 2) % 10), true);
        setattribute("SPD", 20 + ((uh >> 3) % 10), true);
        setattribute("CRIT", 10 + ((uh >> 4) % 10), true);
        setattribute("CRIT_D", 150 + ((uh >> 5) % 100), true);
        setattribute("C_AP", 20 + ((uh >> 3) % 10), true);
        setaegis();
    }
    else {
        setattribute("HP", 0, true);
        setattribute("MAX_HP", get_attribute("HP"), false);
        setattribute("ATK", 0, true);
        setattribute("DEF", 0, true);
        setattribute("SPD", 0, true);
        setattribute("CRIT", 0, true);
        setattribute("CRIT_D", 150, true);
        setattribute("C_AP", 20, true);
    }
}

void character::outputattr() {
    if (!GetRule(SHOW_ATTRIBUTES)) {
        std::cout << "Attributes are hidden." << std::endl;
        return;
    }
    std::cout << textcolor(color::yellow) << "Attributes of " << name_ << ":" << resetcolor() << std::endl;
    for (const auto& attr : attribute_) {
        if (!attr.second.second) continue;
        std::cout << std::setw(10) << std::left << attr.first << ": " << attr.second.first << std::endl;
    }
    if (!aegis.empty()) for (const auto& a : aegis) {
        std::cout << textcolor(color::magenta) << "Aegis: " << a << resetcolor() << std::endl;
    }
}

character::~character() = default;

// -------------------- Buff 系统实现 --------------------

void character::add_buff(const std::string& buff_name, int effect, int duration) {
    if(buffs_[buff_name].second == 0) buffs_[buff_name] = { effect, duration };
    if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::magenta) << name_ << " 获得了 " << buff_name
            << " (效果: " << effect << ", 持续: " << duration << " 回合)" << resetcolor() << std::endl;
    }
}

void character::apply_buffs() {
    if (buffs_.empty()) return;

    for (const auto& pair : buffs_) {
        const std::string& buff_name = pair.first;
        int effect = pair.second.first;

        if (buff_name == "DEFENSE_BUFF") {
            // 已在 take_damage 处理
        }
        else if (buff_name == "POISON" || buff_name == "BURN") {
            int damage = effect;
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(buff_name == "POISON" ? color::green : color::red)
                          << name_ << " 受到" << (buff_name == "POISON" ? "中毒" : "燃烧") << "伤害: "
                          << damage << resetcolor() << std::endl;
            }
            int current_hp = get_attribute("HP");
            setattribute("HP", std::max(0, current_hp - damage), false);
            damage_taken += damage;
            if (get_attribute("HP") == 0 && !is_alive()) {
                if (!GetRule(BATTLE_WITHOUT_OUTPUT))
                    std::cout << textcolor(color::magenta) << bold() << name_
                              << " 因" << (buff_name == "POISON" ? "中毒" : "燃烧") << "而被击败了！" << resetcolor() << std::endl;
            }
        }
        else if (buff_name == "ATTACK_BUFF") {
            if (!GetRule(BATTLE_WITHOUT_OUTPUT))
                std::cout << textcolor(color::yellow) << name_ << " 的攻击力提升！" << resetcolor() << std::endl;
        }
        else if (buff_name == "SPD_BUFF") {
            int bonus_ap = effect;
            int current_ap = get_attribute("C_AP");
            setattribute("C_AP", current_ap + bonus_ap, false);
            if (!GetRule(BATTLE_WITHOUT_OUTPUT))
                std::cout << textcolor(color::cyan) << name_ << " 获得 " << bonus_ap << " 点额外行动点！" << resetcolor() << std::endl;
        }
        else if (buff_name == "SELF_HEAL") {
            int heal_hp = std::min(effect, get_attribute("MAX_HP") - get_attribute("HP"));
            setattribute("HP", get_attribute("HP") + heal_hp);
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                const auto& theme = get_console_theme();
                std::cout << adaptive_textcolor(theme.text) << name_ << " 回复 "
                          << adaptive_textcolor(theme.heal) << heal_hp
                          << adaptive_textcolor(theme.text) << " 点生命！" << resetcolor() << std::endl;
            }
        }
        else if (buff_name == "DRAIN") { // 被吸血时的特殊标记，effect表示吸血量 
            int drain_amount = effect;
            setattribute("HP", get_attribute("HP") - drain_amount);
            //防止HP变成负数
            if (get_attribute("HP") < 0) {
                setattribute("HP", 0);
            }
            //吸血者获得生命
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                const auto& theme = get_console_theme();
                std::cout << adaptive_textcolor(theme.text) << name_ << " 被吸取了 "
                          << adaptive_textcolor(theme.special) << drain_amount
                          << adaptive_textcolor(theme.text) << " 点生命！" << resetcolor() << std::endl;
            }

        }
        else if (buff_name == "STUNNED") {
            // 眩晕效果将在 do_action 中体现，直接在这里标记即可
        }

    }
}

void character::update_buffs() {
    std::vector<std::string> expired_buffs;
    for (auto& pair : buffs_) {
        pair.second.second--;
        if (pair.second.second <= 0) {
            expired_buffs.push_back(pair.first);
        }
    }
    for (const auto& buff_name : expired_buffs) {
        buffs_.erase(buff_name);
        if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
            std::cout << textcolor(color::cyan) << name_ << " 的 " << buff_name
                << " 效果消失了。" << resetcolor() << std::endl;
        }
    }
}

// -------------------- Team 实现 --------------------

void Team::add_character(character& c) {
    members_.push_back(std::ref(c));
}

void Team::remove_character(character& c) {
    auto it = std::find_if(members_.begin(), members_.end(),
        [&c](const std::reference_wrapper<character>& ref) {
            return &ref.get() == &c;
        });
    if (it != members_.end()) members_.erase(it);
}

bool Team::is_alive() const {
    for (const auto& ref : members_) {
        if (ref.get().is_alive()) return true;
    }
    return false;
}

const std::vector<std::reference_wrapper<character>>& Team::get_members() const {
    return members_;
}

std::vector<std::reference_wrapper<character>> Team::get_alive_members() const {
    std::vector<std::reference_wrapper<character>> alive;
    for (const auto& ref : members_) {
        if (ref.get().is_alive()) alive.push_back(ref);
    }
    return alive;
}

void Team::output_team_info() const {
    std::cout << textcolor(color::blue) << "Team: " << name_ << resetcolor() << std::endl;
    for (const auto& ref : members_) {
        character& c = ref.get();
        std::cout << " - " << c.get_name() << (c.is_alive() ? " (Alive)" : " (Defeated)") << std::endl;
        c.outputattr();
    }
}

size_t Team::size() const {
    return members_.size();
}

void Team::clear() {
    members_.clear();
}

void Team::set_name(std::string name) {
    name_ = name;
}

// -------------------- FightComponent 实现 --------------------

void FightComponent::add_team(Team& team) {
    teams_.push_back(std::ref(team));
}

void FightComponent::start() {
    summoned_characters_.clear();
    build_queue();
    int turn_count = 0;
    const int MAX_TURNS = 200000;
    while (!finished_) {
        process_turn();
        ++turn_count;
        if (turn_count > MAX_TURNS) {
            std::cout << customio::textcolor(customio::color::yellow)
                << "[WARN] reached max turn limit (" << MAX_TURNS << "), aborting battle as draw." << customio::resetcolor() << std::endl;
            finished_ = true;
            winner_ = nullptr;
            break;
        }
    }
}

void FightComponent::build_queue() {
    while (!queue_.empty()) queue_.pop();
    for (const auto& team_ref : teams_) {
        const Team& team = team_ref.get();
        for (const auto& ref : team.get_members()) {
            character& c = ref.get();
            if (c.is_alive()) {
                int ap = c.get_attribute("C_AP");
                queue_.push({ &c, ap });
            }
        }
    }
}

void FightComponent::process_turn() {
    if (queue_.empty()) {
        recover_ap();
        build_queue();
        if (queue_.empty()) {
            finished_ = true;
            winner_ = nullptr;
        }
        return;
    }
    ActionNode top = queue_.top();
    queue_.pop();

    if (!top.actor->is_alive()) {
        process_turn();
        return;
    }

    const int min_cost = 10;
    if (top.current_ap < min_cost) {
        recover_ap();
        build_queue();
        return;
    }

    FightContext ctx;
    ctx.summoned = &summoned_characters_;
    const Team* actor_team = get_team_of(*top.actor);
    if (actor_team) {
        for (const auto& team_ref : teams_) {
            const Team& team = team_ref.get();
            for (const auto& ref : team.get_members()) {
                character& c = ref.get();
                ctx.char_team[&c] = &team;
                if (!c.is_alive()) continue;
                if (&team == actor_team) {
                    ctx.allies.push_back(&c);
                }
                else {
                    ctx.enemies.push_back(&c);
                }
            }
        }
    }

    top.actor->apply_buffs();
    bool acted = top.actor->do_action(ctx);
    if (!acted) {
        int old_ap = top.actor->get_attribute("C_AP");
        int new_ap = std::max(0, old_ap - 1);
        top.actor->setattribute("C_AP", new_ap, false);
    }
    top.actor->update_buffs();
    if (top.actor->is_alive()) {
        int new_ap = top.actor->get_attribute("C_AP");
        queue_.push({ top.actor, new_ap });
    }
    check_win_condition();
}

void FightComponent::recover_ap() {
    for (const auto& team_ref : teams_) {
        const Team& team = team_ref.get();
        for (const auto& ref : team.get_members()) {
            character& c = ref.get();
            if (c.is_alive()) {
                int spd = c.get_attribute("SPD");
                if (spd <= 0) spd = 20;
                int old_ap = c.get_attribute("C_AP");
                int new_ap = old_ap + spd;
                c.setattribute("C_AP", new_ap, false);
            }
        }
    }
}

void FightComponent::check_win_condition() {
    int alive_teams = 0;
    const Team* last_alive = nullptr;
    for (const auto& team_ref : teams_) {
        const Team& team = team_ref.get();
        if (team.is_alive()) {
            ++alive_teams;
            last_alive = &team;
        }
    }
    if (alive_teams <= 1) {
        finished_ = true;
        winner_ = (alive_teams == 1) ? last_alive : nullptr;
    }
}

const Team* FightComponent::get_team_of(const character& c) const {
    for (const auto& team_ref : teams_) {
        const Team& team = team_ref.get();
        for (const auto& ref : team.get_members()) {
            if (&ref.get() == &c) return &team;
        }
    }
    return nullptr;
}

// ==================== 角色导入系统实现 ====================

std::unordered_map<std::string, ImportedCharacterData> imported_characters;

bool import_characters_from_json(const std::string& json_file_path) {
    std::ifstream file(json_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << textcolor(color::red) << "错误：无法打开文件 " << json_file_path << resetcolor() << std::endl;
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    if (content.size() >= 3 && 
        (unsigned char)content[0] == 0xEF && 
        (unsigned char)content[1] == 0xBB && 
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3);
    }
    imported_characters.clear();
    size_t char_start = content.find("\"characters\"");
    if (char_start == std::string::npos) {
        std::cout << textcolor(color::red) << "错误：JSON 文件中未找到 characters 字段" << resetcolor() << std::endl;
        return false;
    }
    size_t array_start = content.find('[', char_start);
    if (array_start == std::string::npos) {
        std::cout << textcolor(color::red) << "错误：JSON 格式错误" << resetcolor() << std::endl;
        return false;
    }
    size_t array_end = array_start;
    int bracket_depth = 0;
    bool in_string = false;
    for (size_t i = array_start; i < content.size(); ++i) {
        char ch = content[i];
        if (ch == '"') {
            bool escaped = false;
            size_t j = i;
            while (j > array_start && content[--j] == '\\') escaped = !escaped;
            if (!escaped) in_string = !in_string;
        }
        if (in_string) continue;
        if (ch == '[') {
            bracket_depth++;
        } else if (ch == ']') {
            bracket_depth--;
            if (bracket_depth == 0) {
                array_end = i;
                break;
            }
        }
    }
    if (array_end == array_start || bracket_depth != 0) {
        std::cout << textcolor(color::red) << "错误：JSON 格式错误" << resetcolor() << std::endl;
        return false;
    }
    std::string array_content = content.substr(array_start + 1, array_end - array_start - 1);
    size_t obj_start = 0;
    int count = 0;
    while (true) {
        obj_start = array_content.find('{', obj_start);
        if (obj_start == std::string::npos) break;
        size_t obj_end = obj_start;
        int brace_depth = 0;
        in_string = false;
        for (size_t i = obj_start; i < array_content.size(); ++i) {
            char ch = array_content[i];
            if (ch == '"') {
                bool escaped = false;
                size_t j = i;
                while (j > obj_start && array_content[--j] == '\\') escaped = !escaped;
                if (!escaped) in_string = !in_string;
            }
            if (in_string) continue;
            if (ch == '{') {
                brace_depth++;
            } else if (ch == '}') {
                brace_depth--;
                if (brace_depth == 0) {
                    obj_end = i;
                    break;
                }
            }
        }
        if (obj_end == obj_start) break;
        std::string obj_str = array_content.substr(obj_start + 1, obj_end - obj_start - 1);
        ImportedCharacterData data;
        std::string id;
        size_t id_pos = obj_str.find("\"id\"");
        if (id_pos != std::string::npos) {
            size_t colon_pos = obj_str.find(':', id_pos);
            size_t quote_start = obj_str.find('\"', colon_pos);
            size_t quote_end = obj_str.find('\"', quote_start + 1);
            if (quote_start != std::string::npos && quote_end != std::string::npos) {
                id = obj_str.substr(quote_start + 1, quote_end - quote_start - 1);
            }
        }
        size_t name_pos = obj_str.find("\"name\"");
        if (name_pos != std::string::npos) {
            size_t colon_pos = obj_str.find(':', name_pos);
            size_t quote_start = obj_str.find('\"', colon_pos);
            size_t quote_end = obj_str.find('\"', quote_start + 1);
            if (quote_start != std::string::npos && quote_end != std::string::npos) {
                data.name = Utf8ToAnsi( obj_str.substr(quote_start + 1, quote_end - quote_start - 1));
            }
        }
        std::vector<std::string> attrs = {"HP", "MP", "ATK", "DEF", "MATK", "SPD", "CRIT", "CRIT_D", "C_AP"};
        for (const auto& attr : attrs) {
            std::string search_str = "\"" + attr + "\"";
            size_t attr_pos = obj_str.find(search_str);
            if (attr_pos != std::string::npos) {
                size_t colon_pos = obj_str.find(':', attr_pos);
                size_t end_pos = obj_str.find(',', colon_pos);
                if (end_pos == std::string::npos) {
                    end_pos = obj_str.find('}', colon_pos);
                }
                std::string value_str = obj_str.substr(colon_pos + 1, end_pos - colon_pos - 1);
                value_str.erase(remove_if(value_str.begin(), value_str.end(), isspace), value_str.end());
                try {
                    int value = std::stoi(value_str);
                    data.attributes[attr] = value;
                } catch (...) {}
            }
        }
        size_t actions_pos = obj_str.find("\"actions\"");
        if (actions_pos != std::string::npos) {
            size_t colon_pos = obj_str.find(':', actions_pos);
            size_t array_start = obj_str.find('[', colon_pos);
            size_t array_end = obj_str.find(']', array_start);
            if (array_start != std::string::npos && array_end != std::string::npos) {
                std::string actions_str = obj_str.substr(array_start + 1, array_end - array_start - 1);
                size_t pos = 0;
                while (true) {
                    size_t quote_start = actions_str.find('\"', pos);
                    if (quote_start == std::string::npos) break;
                    size_t quote_end = actions_str.find('\"', quote_start + 1);
                    if (quote_end == std::string::npos) break;
                    std::string action_name = actions_str.substr(quote_start + 1, quote_end - quote_start - 1);
                    data.actions.push_back(action_name);
                    pos = quote_end + 1;
                }
            }
        }
        if (!id.empty()) {
            imported_characters[id] = data;
            std::cout << textcolor(color::green) << "已导入角色: [" << id << "] " << data.name << resetcolor() << std::endl;
            count++;
        }
        obj_start = obj_end + 1;
    }
    if (count == 0) {
        std::cout << textcolor(color::yellow) << "警告：未导入任何角色" << resetcolor() << std::endl;
        return false;
    }
    std::cout << textcolor(color::green) << "成功导入 " << count << " 个角色" << resetcolor() << std::endl;
    return true;
}

bool has_imported_character(const std::string& char_id) {
    return imported_characters.find(char_id) != imported_characters.end();
}

const ImportedCharacterData* get_imported_character(const std::string& char_id) {
    auto it = imported_characters.find(char_id);
    if (it != imported_characters.end()) {
        return &it->second;
    }
    return nullptr;
}

// 武器系统实现 

bool character::equip_weapon(WeaponData& weapon) {
    if (weapon.equipped) return false;
    if (!equipped_weapon_id_.empty()) unequip_weapon();

    // 应用属性加成
    for (const auto& [attr, value] : weapon.attributes) {
        int current = get_attribute(attr);
        setattribute(attr, current + value);
        weapon_bonus_[attr] = value;
    }

    // 如果武器提供额外技能，添加到技能列表并记录指针
    if (!weapon.skill_grant.empty()) {
        const SkillInfo* info = SkillRegistry::getSkillInfo(weapon.skill_grant);
        if (info) {
            auto new_action = info->factory();
            weapon_granted_action_ = new_action.get();  // 保存原始指针
            actions_.push_back(std::move(new_action));
        }
    }

    equipped_weapon_id_ = weapon.id;
    weapon.equipped = true;
    return true;
}

std::string character::unequip_weapon() {
    std::string old_id = equipped_weapon_id_;
    if (old_id.empty()) return "";

    // 还原属性
    for (const auto& [attr, bonus] : weapon_bonus_) {
        int current = get_attribute(attr);
        setattribute(attr, current - bonus);
    }
    weapon_bonus_.clear();

    // 精准移除武器授予的技能
    if (weapon_granted_action_ != nullptr) {
        auto it = std::find_if(actions_.begin(), actions_.end(),
            [this](const std::unique_ptr<act>& ptr) {
                return ptr.get() == weapon_granted_action_;
            });
        if (it != actions_.end()) {
            actions_.erase(it);
        }
        weapon_granted_action_ = nullptr;
    }

    equipped_weapon_id_.clear();
    return old_id;
}

const WeaponData* character::get_equipped_weapon(const std::vector<WeaponData>& weapon_library) const {
    if (equipped_weapon_id_.empty()) return nullptr;
    for (const auto& w : weapon_library) {
        if (w.id == equipped_weapon_id_) return &w;
    }
    return nullptr;
}

double character::get_benchmark_score() const {
    if (!is_benchmark_cached()) {
        ensure_benchmark_cached(const_cast<character&>(*this));
    }
    return benchmark_score_;
}

std::string character::get_benchmark_grade() const {
    if (!is_benchmark_cached()) {
        ensure_benchmark_cached(const_cast<character&>(*this));
    }
    return benchmark_grade_;
}