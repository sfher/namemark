// game.h
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
    LOBBY,
    TEAM,
    ADVENTURE,
    SHOP,
    GACHA,
    CONSOLE,
    SETTINGS,
    TEAM_TEST,
    EXIT
};

// 跨状态共享上下文

struct GameContext {
    std::vector<WeaponData> weapon_templates;   // 武器模板库（只读，供商店和抽卡引用）
    std::vector<std::unique_ptr<character>> all_characters;
    std::vector<size_t> selected_team;
    LevelManager level_manager;
    MonsterPresetManager preset_manager;
    PackageManager package_manager;

    int gold = 0;                                    // 全局金币
    std::vector<WeaponData> weapons;                  // 当前模组的武器库

     // 抽卡
    int gacha_single_cost = 300;
    int gacha_ten_cost = 2800;
    std::unordered_map<std::string, double> gacha_rates;  // SSR -> 0.05 等
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> gacha_pools; // 稀有度 -> [(type, id)]
};

// 状态基类
class GameState {
public:
    explicit GameState(GameContext& ctx) : ctx_(ctx) {}
    virtual ~GameState() = default;

    // 进入状态时调用
    virtual void on_enter() {}
    // 每帧更新（目前是每次输入后更新）
    virtual void update() = 0;
    // 退出状态时调用
    virtual void on_exit() {}

protected:
    GameContext& ctx_;
};

// 游戏主类（单例）
class Game {
public:
    static Game& getInstance();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    PackageManager package_manager_;

    void run();
    void changeState(GameStateType type);
    void quit() { running_ = false; }
    GameContext& getContext() { return ctx_; }

private:
    Game();   // 仅声明，不定义
    ~Game() = default;

    bool running_ = true;
    GameContext ctx_;
    std::unique_ptr<GameState> current_state_;
    GameStateType current_type_ = GameStateType::EXIT;
};