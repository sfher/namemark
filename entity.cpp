#include "entity.h"
#include "customio.h"
#include "act.h"       // 需要完整定义 act 及其派生类
#include <iomanip>
#include <random>
#include <chrono>
#include <climits>

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

character::character(std::string name) : name_(name) {
    SetRule(SUMMON_BASIC_ATTR, true);
    SetRule(SHOW_ATTRIBUTES, true);
    setbasicattr();
    init_default_actions();
}

void character::setaegis() {    //设置加护
    if (hashcode % 10000 == 0) {
        aegis.push_back("Sovereign"); //主宰!
        setattribute("MATK", 0); //代价就是没有法伤
        setattribute("MP", 0);
        setattribute("ATK", get_attribute("ATK") + 20);
        setattribute("CRIT", get_attribute("CRIT") + 50);
        setattribute("CRIT_D", get_attribute("CRIT_D") - 20);
        setattribute("DEF", get_attribute("DEF") + 20);
        setattribute("HP", std::min(get_attribute("HP") + 40, 220));
		setattribute("MAX_HP", std::min(get_attribute("MAX_HP") + 40, 220));
        setattribute("SPD", std::min(get_attribute("SPD") * 4, 80)); //极高的速度加成，但有上限
        setattribute("C_AP", std::min(get_attribute("C_AP") + 60, 80)); //增加初始行动点，但有上限
		buffs_["DEFENSE_BUFF"] = { 40, 99 }; //获得防御增益buff，效果40%，持续99回合（相当于永久）
    }
        else if (hashcode % 10000 == 1) {
            aegis.push_back("Guardian"); //守护者
            setattribute("MATK", get_attribute("MATK") - 10);
            setattribute("ATK", get_attribute("ATK") - 10);
            setattribute("DEF", get_attribute("DEF") + 30);
            setattribute("HP", std::min(get_attribute("HP") + 50, 250));
            setattribute("MAX_HP", std::min(get_attribute("MAX_HP") + 50, 250));
            buffs_["DEFENSE_BUFF"] = { 93, 99 }; //获得防御增益buff
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

    // 使用角色名字哈希作为种子，保证同一角色技能集稳定
    uint64_t role_seed = std::hash<std::string>{}(name_);
    std::mt19937 rng(static_cast<unsigned>(role_seed));

    for (const auto& pair : sorted_skills) {
        const SkillInfo& info = pair.second;
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
    // 收集所有可执行技能及其权重
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

    // 加权随机选择
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
    if (damage <= 0) return;
    
    // 检查buff系统中的防御增益
    int def_buff = 0;
    auto buff_it = buffs_.find("DEFENSE_BUFF");
    if (buff_it != buffs_.end()) {
        def_buff = buff_it->second.first;
    }
    
    // 如果buff系统中没有，检查属性（向后兼容）
    if (def_buff == 0) {
        def_buff = get_attribute("DEFENSE_BUFF");
    }
    
    // 应用防御 buff（简单实现）
    if (def_buff > 0) {
        damage = damage * (100 - def_buff) / 100;
        if (damage <= 0) damage = 1;
        // 从属性中清除（如果使用了属性方式）
        if (get_attribute("DEFENSE_BUFF") > 0) {
            setattribute("DEFENSE_BUFF", 0, false);
        }
    }
    
    int current_hp = get_attribute("HP");
    int new_hp = current_hp - damage;
    setattribute("HP", std::max(0, new_hp));
    if (!GetRule(BATTLE_WITHOUT_OUTPUT))
        std::cout << textcolor(color::cyan) << name_ << " 受到了 " << damage
        << " 点伤害，剩余HP：" << get_attribute("HP") << resetcolor() << std::endl;
    if (get_attribute("HP") == 0) {
        if (!GetRule(BATTLE_WITHOUT_OUTPUT))
            std::cout << textcolor(color::magenta) << bold() << name_ << " 被击败了！" << resetcolor() << std::endl;
    }
}

void character::attack(entity& target) {
    // 保留用于旧系统，新战斗不再使用
    character* rival = dynamic_cast<character*>(&target);
    if (!rival) {
        if (!GetRule(BATTLE_WITHOUT_OUTPUT))
            std::cout << textcolor(color::red) << "攻击目标不是有效角色！" << resetcolor() << std::endl;
        return;
    }
    if (!is_alive() || !rival->is_alive()) return;
    int damage = calculate_damage(*rival);
    rival->take_damage(damage);
}

int character::calculate_damage(character& target) {
    // 保留用于评分系统（可保留）
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
        setattribute("MAX_HP", get_attribute("HP"), false); //复制最大生命
        setattribute("MP", 20 + ((uh >> 7) % 20), true);
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
        setattribute("MAX_HP", get_attribute("HP"), false); //复制最大生命
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
    //输出加护
    if(!aegis.empty()) for (const auto& a : aegis) {
        std::cout << textcolor(color::magenta) << "Aegis: " << a << resetcolor() << std::endl;
	}
}

character::~character() = default;

// -------------------- Buff 系统实现 --------------------

void character::add_buff(const std::string& buff_name, int effect, int duration) {
    // 如果buff已存在，更新其效果和时间
    buffs_[buff_name] = { effect, duration };
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

        // 根据buff名称应用不同的效果
        if (buff_name == "DEFENSE_BUFF") {
            // 防御增益：伤害减少（这个已在take_damage中实现，此处可以增强显示）
        }
        else if (buff_name == "POISON") {
            // 中毒：每回合损失HP
            int damage = effect;
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::green) << name_ << " 受到中毒伤害: " 
                    << damage << resetcolor() << std::endl;
            }
            // 直接扣血（不走take_damage以避免防御buff干扰）
            int current_hp = get_attribute("HP");
            setattribute("HP", std::max(0, current_hp - damage), false);
            if (get_attribute("HP") == 0 && is_alive() == false) {
                if (!GetRule(BATTLE_WITHOUT_OUTPUT))
                    std::cout << textcolor(color::magenta) << bold() << name_ 
                        << " 因中毒而被击败了！" << resetcolor() << std::endl;
            }
        }
        else if (buff_name == "BURN") {
            // 燃烧：每回合损失HP（比中毒伤害更高）
            int damage = effect;
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::red) << name_ << " 受到燃烧伤害: " 
                    << damage << resetcolor() << std::endl;
            }
            int current_hp = get_attribute("HP");
            setattribute("HP", std::max(0, current_hp - damage), false);
            if (get_attribute("HP") == 0 && is_alive() == false) {
                if (!GetRule(BATTLE_WITHOUT_OUTPUT))
                    std::cout << textcolor(color::magenta) << bold() << name_ 
                        << " 因燃烧而被击败了！" << resetcolor() << std::endl;
            }
        }
        else if (buff_name == "ATTACK_BUFF") {
            // 攻击增益：在属性计算中已生效（需在技能中使用）
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::yellow) << name_ 
                    << " 的攻击力提升！" << resetcolor() << std::endl;
            }
        }
        else if (buff_name == "SPD_BUFF") {
            // 速度增益：下一回合获得额外AP
            int bonus_ap = effect;
            int current_ap = get_attribute("C_AP");
            setattribute("C_AP", current_ap + bonus_ap, false);
            if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
                std::cout << textcolor(color::cyan) << name_ 
                    << " 获得 " << bonus_ap << " 点额外行动点！" << resetcolor() << std::endl;
            }
        }
    }
}

void character::update_buffs() {
    std::vector<std::string> expired_buffs;
    
    // 更新所有buff的持续时间
    for (auto& pair : buffs_) {
        pair.second.second--;  // 减少持续时间
        if (pair.second.second <= 0) {
            expired_buffs.push_back(pair.first);
        }
    }

    // 移除过期的buff
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

void FightComponent::start() { //开始游戏~
    build_queue();
    // Safety: guard against extremely long/ infinite battles by limiting turns
    int turn_count = 0;
    const int MAX_TURNS = 200000; // 可调：超过此值视为僵局，强制结束
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

void FightComponent::build_queue() { //创建队列
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

    // 获取最小消耗（临时硬编码为 10，防御消耗）
    const int min_cost = 10;
    if (top.current_ap < min_cost) {
        // 所有角色 AP 都不足，全局恢复一次
        recover_ap();
        build_queue();
        return;
    }

    // 构建战斗上下文（同现有代码）
    FightContext ctx;
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

    // 执行行为
    // 在详细模式下输出当前行动者（帮助定位卡住）
    if (!top.actor->GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::white) << "[TURN] " << top.actor->get_name()
            << "  AP=" << top.actor->get_attribute("C_AP") << resetcolor() << std::endl;
    }

    // 应用buff效果（在该角色行动前）
    top.actor->apply_buffs();

    bool acted = top.actor->do_action(ctx);
    if (!acted) {
        // 如果没有动作实际执行，安全地消耗部分 AP，避免队列卡住
        int old_ap = top.actor->get_attribute("C_AP");
        int new_ap = std::max(0, old_ap - 1);
        top.actor->setattribute("C_AP", new_ap, false);
    }

    // 更新buff持续时间（在行动后）
    top.actor->update_buffs();

    // 重新入队
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