// states/lobby_state.cpp
#include "lobby_state.h"
#include "../customio.h"
#include <iostream>

using namespace customio;

LobbyState::LobbyState(GameContext& ctx) : GameState(ctx) {}

void LobbyState::on_enter() {
    // 标题由 menu_select 内部清屏后重新显示，此处可留空
}

void LobbyState::update() {
    std::vector<std::string> items = {
        "队伍管理",
        "队伍测试",
        "神秘商店",
        "抽卡模式",
        "开始冒险",
        "设置模式",
        "[控制台]",
        "退出游戏"
    };
    int choice = menu_select(items, "^接下来做点什么呢");
    switch (choice) {
    case 0: Game::getInstance().changeState(GameStateType::TEAM); break;
    case 1: Game::getInstance().changeState(GameStateType::TEAM_TEST); break;
    case 2: Game::getInstance().changeState(GameStateType::SHOP); break;
    case 3: Game::getInstance().changeState(GameStateType::GACHA); break;
    case 4: Game::getInstance().changeState(GameStateType::ADVENTURE); break;
    case 5: Game::getInstance().changeState(GameStateType::SETTINGS); break;
    case 6: Game::getInstance().changeState(GameStateType::CONSOLE); break;
    case 7: Game::getInstance().changeState(GameStateType::EXIT); break;
    default: break;
    }
}