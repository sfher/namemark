// game.cpp
#include "game.h"
#include "states/lobby_state.h"
#include "states/team_state.h"
#include "states/adventure_state.h"
#include "customio.h"
#include "console.h"
#include <iostream>
#include <conio.h>

using namespace customio;

Game::Game() {
    ctx_.preset_manager.load_from_json("monsters.json");
    ctx_.level_manager.set_preset_manager(&ctx_.preset_manager);
    ctx_.level_manager.load_from_json("levels.json");
}

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

void Game::changeState(GameStateType type) {
    if (current_state_) {
        current_state_->on_exit();
        current_state_.reset();
    }
    switch (type) {
    case GameStateType::LOBBY:
        current_state_ = std::make_unique<LobbyState>(ctx_);
        break;
    case GameStateType::TEAM:
        current_state_ = std::make_unique<TeamState>(ctx_);
        break;
    case GameStateType::ADVENTURE:
        current_state_ = std::make_unique<AdventureState>(ctx_);
        break;
    case GameStateType::CONSOLE:
        debug_console();
        break;
    case GameStateType::EXIT:
        running_ = false;
        return;
    }
    current_type_ = type;
    if (current_state_) {
        current_state_->on_enter();
    }
}

void Game::run() {
    // 关卡已在构造函数中加载，此处不再重复加载
    changeState(GameStateType::LOBBY);
    while (running_ && current_state_) {
        clear_screen();
        current_state_->update();
        if (!running_) break;
        std::cout << "\n按任意键继续...";
        _getch();
    }
}