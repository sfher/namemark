// entity.h
#ifndef ENTITY_H_
#define ENTITY_H_

#include "weapon_data.h"
// 移除 #include "console.h"  ← 关键修改
#include "act.h"
#include <string>
#include <deque>
#include <unordered_map>
#include <random>
#include <vector>
#include <functional>
#include <algorithm>
#include <queue>
#include <memory>

class act;
struct FightContext;
class Team;
class character;

enum rule {
    SUMMON_BASIC_ATTR = 0x01,
    SHOW_ATTRIBUTES = 0x02,
    TESTED = 0x03,
    SLOW_BATTLE = 0x04,
    BATTLE_WITHOUT_OUTPUT = 0x05
};

class entity {
public:
    entity() = default;
    virtual ~entity() = default;
    virtual bool is_alive() const = 0;
    virtual void attack(entity& target) = 0;
    virtual void take_damage(int damage) = 0;
    virtual std::string get_name() const = 0;
    virtual int get_attribute(const std::string& attr_name) const = 0;
};

class stereotype : public entity {
protected:
    std::unordered_map<int, bool> rule_;
public:
    stereotype() = default;
    virtual ~stereotype() = default;
    void SetRule(int ruleID, bool enabled) { rule_[ruleID] = enabled; }
    bool GetRule(int ruleID) const {
        auto it = rule_.find(ruleID);
        return it != rule_.end() ? it->second : false;
    }
};

class character : public stereotype {
private:
    int calculate_damage(character& target);
    static std::mt19937& get_random_engine();
    act* weapon_granted_action_ = nullptr;
    std::unordered_map<std::string, int> weapon_bonus_;

    std::string name_ = "character";
    int64_t hashcode = 0;
    std::unordered_map<std::string, std::pair<int, bool>> attribute_{};
    std::vector<std::string> aegis;
    std::vector<std::unique_ptr<act>> actions_;
    std::unordered_map<std::string, std::pair<int, int>> buffs_;

    double benchmark_score_ = -1.0;
    std::string benchmark_grade_ = "?";
public:
    character();
    explicit character(std::string name);
    ~character();

    void setattribute(const std::string& attr_name, int value, bool show);
    void setattribute(const std::string& attr_name, int value);
    int get_attribute(const std::string& attr_name) const override;
    std::string get_name() const override;
    void set_name(std::string name);
    bool is_alive() const override;
    void take_damage(int damage) override;
    void take_damage(int damage, character* attacker);
    void attack(entity& target) override;
    void setbasicattr();
    void outputattr();
    void setaegis();
    void add_buff(const std::string& buff_name, int effect, int duration);
    void apply_buffs();
    void update_buffs();
    void init_default_actions();
    bool do_action(FightContext& ctx);
    void add_action(std::unique_ptr<act> action) { actions_.push_back(std::move(action)); }
    const std::vector<std::unique_ptr<act>>& get_actions() const { return actions_; }
    std::vector<std::unique_ptr<act>>& get_actions() { return actions_; }
    bool has_aegis() const { return !aegis.empty(); }
    std::vector<std::string> get_aegis() const { return aegis; }
    std::unordered_map<std::string, std::pair<int, int>> get_buffs() const { return buffs_; }
    void set_buffs(const std::unordered_map<std::string, std::pair<int, int>>& new_buffs) { buffs_ = new_buffs; }
    std::unique_ptr<character> clone_for_battle() const;

    // ---------- 战斗统计 ----------
    int damage_dealt = 0;
    int damage_taken = 0;
    int kills = 0;
    int healing_done = 0;
    void reset_stats() { damage_dealt = damage_taken = kills = healing_done = 0;}

    bool is_benchmark_cached() const { return benchmark_score_ >= 0.0; }

    // 声明，实现在 entity.cpp
    double get_benchmark_score() const;
    std::string get_benchmark_grade() const;

    void set_benchmark_score(double s) { benchmark_score_ = s; }
    void set_benchmark_grade(const std::string& g) { benchmark_grade_ = g; }

    // ---------- 武器系统 ----------
    std::string equipped_weapon_id_;
    bool equip_weapon(WeaponData& weapon);
    std::string unequip_weapon();
    const WeaponData* get_equipped_weapon(const std::vector<WeaponData>& weapon_library) const;
    bool has_weapon() const { return !equipped_weapon_id_.empty(); }
};

class Team {
private:
    std::string name_;
    std::vector<std::reference_wrapper<character>> members_;
public:
    Team() = default;
    explicit Team(const std::string& name) : name_(name) {}

    void add_character(character& c);
    void remove_character(character& c);
    bool is_alive() const;
    const std::vector<std::reference_wrapper<character>>& get_members() const;
    std::vector<std::reference_wrapper<character>> get_alive_members() const;
    void output_team_info() const;
    size_t size() const;
    void clear();
    const std::string& get_name() const { return name_; }
    void set_name(std::string name);
};

struct FightContext {
    std::vector<character*> enemies;
    std::vector<character*> allies;
    std::unordered_map<character*, const Team*> char_team;
    std::deque<std::unique_ptr<character>>* summoned = nullptr;
};

class FightComponent {
public:
    void add_team(Team& team);
    void start();

private:
    struct ActionNode {
        character* actor;
        int current_ap;
        bool operator<(const ActionNode& other) const {
            return current_ap < other.current_ap;
        }
    };

    std::vector<std::reference_wrapper<Team>> teams_;
    std::priority_queue<ActionNode> queue_;
    std::deque<std::unique_ptr<character>> summoned_characters_;   // 原为 std::vector
    bool finished_ = false;
    const Team* winner_ = nullptr;

    void build_queue();
    void process_turn();
    void recover_ap();
    void check_win_condition();
    const Team* get_team_of(const character& c) const;
};

// ==================== 角色导入系统 ====================
struct ImportedCharacterData {
    std::string name;
    std::unordered_map<std::string, int> attributes;
    std::vector<std::string> actions;
};

extern std::unordered_map<std::string, ImportedCharacterData> imported_characters;

bool import_characters_from_json(const std::string& json_file_path);
bool has_imported_character(const std::string& char_id);
const ImportedCharacterData* get_imported_character(const std::string& char_id);

#endif