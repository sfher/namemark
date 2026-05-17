// game.h — 游戏状态机与跨状态共享上下文
#pragma once

#include "act.h"
#include "entity.h"
#include "level_data.h"
#include "monster_preset.h"
#include "package_manager.h"
#include "weapon_data.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

// 游戏状态枚举
enum class GameStateType {
    LOBBY,       // 大厅
    TEAM,        // 队伍管理
    ADVENTURE,   // 冒险（关卡选择）
    SHOP,        // 商店
    GACHA,       // 抽卡
    CONSOLE,     // 调试控制台（浮层，不切换状态）
    SETTINGS,    // 设置
    TEAM_TEST,   // 队内演习
    EXIT         // 退出游戏
};

// 跨状态共享上下文（所有状态通过 ctx_ 引用访问）
struct GameContext {
    std::vector<WeaponData> weapon_templates;             // 武器模板库（只读引用）
    std::vector<std::unique_ptr<character>> all_characters; // 已拥有角色
    std::vector<size_t> selected_team;                    // 出战队伍（索引数组）
    LevelManager level_manager;
    MonsterPresetManager preset_manager;
    PackageManager package_manager;

    int gold = 0;
    std::vector<WeaponData> weapons;                      // 已购买武器库

    // 抽卡配置（从模组 gacha.json 加载）
    int gacha_single_cost = 300;
    int gacha_ten_cost = 2800;
    std::unordered_map<std::string, double> gacha_rates;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> gacha_pools;
};

// 状态基类：每个游戏界面实现 on_enter / update / on_exit
class GameState {
public:
    explicit GameState(GameContext& ctx) : ctx_(ctx) {}
    virtual ~GameState() = default;

    virtual void on_enter() {}   // 进入状态
    virtual void update() = 0;   // 每帧调用（显示菜单、处理输入、切换状态）
    virtual void on_exit() {}    // 离开状态

protected:
    GameContext& ctx_;
};

// 游戏主类（单例）：持有状态栈，驱动主循环
class Game {
public:
    static Game& getInstance();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    PackageManager package_manager_;

    void run();                            // 模组选择 → 加载数据 → 进入主循环
    void changeState(GameStateType type);  // 切换到指定状态（CONSOLE 为浮层不销毁当前状态）
    void goBack();                         // 返回状态栈中的上一个状态
    void quit() { running_ = false; }
    GameContext& getContext() { return ctx_; }

private:
    Game();
    ~Game() = default;

    bool running_ = true;
    GameContext ctx_;
    std::unique_ptr<GameState> current_state_;
    GameStateType current_type_ = GameStateType::EXIT;
    std::vector<GameStateType> state_stack_;  // ESC 返回栈
};
