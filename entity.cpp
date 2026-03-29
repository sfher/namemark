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
    setattribute("C_AP", 0, false);
    init_default_actions();
}

character::character(std::string name) : name_(name) {
    SetRule(SUMMON_BASIC_ATTR, true);
    SetRule(SHOW_ATTRIBUTES, true);
    setbasicattr();
    setattribute("C_AP", 0, false);
    init_default_actions();
}

void character::init_default_actions() {
    actions_.push_back(std::make_unique<Attack>());
    actions_.push_back(std::make_unique<Defense>());
    // 可在此添加更多技能
}

void character::act(FightContext& ctx) {
    for (auto& action : actions_) {
        if (action->can_execute(this, ctx)) {
            action->execute(this, ctx);
            return;
        }
    }
    if (!GetRule(BATTLE_WITHOUT_OUTPUT)) {
        std::cout << textcolor(color::yellow) << name_ << " 无法行动！" << resetcolor() << std::endl;
    }
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
    // 应用防御 buff（简单实现）
    int def_buff = get_attribute("DEFENSE_BUFF");
    if (def_buff > 0) {
        damage = damage * (100 - def_buff) / 100;
        if (damage <= 0) damage = 1;
        setattribute("DEFENSE_BUFF", 0, false);
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
        bool lucky = ((uh >> 7) % 100) < 5;
        setattribute("HP", 120 + (uh % 60) + 30 * lucky, true);
        setattribute("ATK", 30 + ((uh >> 1) % 20) + 15 * lucky, true);
        setattribute("DEF", 10 + ((uh >> 2) % 10) + 15 * lucky, true);
        setattribute("SPD", 20 + ((uh >> 3) % 10) + 15 * lucky, true);
        setattribute("CRIT", 10 + ((uh >> 4) % 10) + 15 * lucky, true);
        setattribute("CRIT_D", 150 + ((uh >> 5) % 100) + 50 * lucky, true);
        setattribute("C_AP", 20 + ((uh >> 3) % 10) + 15 * lucky, true);
    }
    else {
        setattribute("HP", 0, true);
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
}

character::~character() = default;

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
    build_queue();
    while (!finished_) {
        process_turn();
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

    // 构建战斗上下文
    FightContext ctx;
    const Team* actor_team = get_team_of(*top.actor);
    if (actor_team) {
        for (const auto& team_ref : teams_) {
            const Team& team = team_ref.get();
            for (const auto& ref : team.get_members()) {
                character& c = ref.get();
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

    // 执行行为（内部会检查资源）
    top.actor->act(ctx);

    // 重新入队（如果还活着）
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

character* FightComponent::select_target(character& actor) {
    // 此函数不再使用，因为目标选择已由行为类处理，但保留以防万一
    const Team* actor_team = get_team_of(actor);
    if (!actor_team) return nullptr;
    for (const auto& team_ref : teams_) {
        const Team& team = team_ref.get();
        if (&team == actor_team) continue;
        for (const auto& ref : team.get_members()) {
            character& c = ref.get();
            if (c.is_alive()) return &c;
        }
    }
    return nullptr;
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