// game.h
#pragma once

#include "act.h"
#include "entity.h"
#include "level_data.h"
#include "monster_preset.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

// 游戏状态枚举
enum class GameStateType {
    LOBBY,
    TEAM,
    ADVENTURE,
    CONSOLE,
    SETTINGS,
    EXIT
};

// 跨状态共享上下文

struct GameContext {
    std::vector<std::unique_ptr<character>> all_characters;
    std::vector<size_t> selected_team;
    LevelManager level_manager;
    MonsterPresetManager preset_manager;
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