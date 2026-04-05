#ifndef ENTITY_H_
#define ENTITY_H_

#include <string>
#include <unordered_map>
#include <random>
#include <vector>
#include <functional>
#include <algorithm>
#include <queue>
#include <memory>       // for unique_ptr

// 前向声明 act 相关类（避免循环依赖）
class act;
struct FightContext;

// 规则枚举
enum rule {
    SUMMON_BASIC_ATTR = 0x01,
    SHOW_ATTRIBUTES = 0x02,
    TESTED = 0x03,
    SLOW_BATTLE = 0x04,
    BATTLE_WITHOUT_OUTPUT = 0x05
};

// 顶层抽象类
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

// 抽象模板类（规则管理）
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

// 前向声明 Team
class Team;

// 具体角色类
class character : public stereotype {
private:
    int calculate_damage(character& target);  // 保留用于评分系统，可废弃
    static std::mt19937& get_random_engine(); // 保留用于评分系统

    std::string name_ = "character";
    int64_t hashcode = 0;
    std::unordered_map<std::string, std::pair<int, bool>> attribute_{};
    std::vector<std::string> aegis; //神之庇护
    std::vector<std::unique_ptr<act>> actions_;  // 行为列表
    std::unordered_map<std::string, std::pair<int, int>> buffs_; // buff系统: buff名字 -> {效果值, 剩余时间}

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
    void attack(entity& target) override;   // 保留用于评分系统
    void setbasicattr();
    void outputattr();
    void setaegis(); //设置加护
    bool has_aegis() const { return !aegis.empty(); }
    const std::vector<std::string>& get_aegis() const { return aegis; }
    // Buff系统接口
    void add_buff(const std::string& buff_name, int effect, int duration);  // 添加buff
    void apply_buffs();                                                      // 应用buff效果（减伤、毒伤等）
    void update_buffs();                                                     // 更新buff持续时间（每回合递减）
    // 新行为系统
    void init_default_actions();            // 初始化默认行为
    bool do_action(FightContext& ctx);            // 执行回合行为，返回是否执行了动作
};

// 队伍类
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

// 战斗上下文（传递给行为）
struct FightContext {
    std::vector<character*> enemies;   // 敌方存活角色列表
    std::vector<character*> allies;    // 我方存活角色列表
    std::unordered_map<character*, const Team*> char_team; // 角色 -> 所属队伍
    // 可扩展其他信息
};

// 战斗组件类
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
    bool finished_ = false;
    const Team* winner_ = nullptr;

    void build_queue();
    void process_turn();
    void recover_ap();
    void check_win_condition();
    const Team* get_team_of(const character& c) const;  // 辅助函数
};

#endif // ENTITY_H_