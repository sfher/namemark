// states/lobby_state.cpp
#include "lobby_state.h"
#include "../customio.h"
#include <iostream>

using namespace customio;

LobbyState::LobbyState(GameContext& ctx) : GameState(ctx) {}

void LobbyState::on_enter() {
    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 冒险者大厅 =====" << resetcolor() << std::endl;
}

void LobbyState::update() {
    const auto& theme = get_console_theme();
    std::cout << "\n";
    std::cout << "  1. 队伍管理\n";
    std::cout << "  2. 开始冒险\n";
    std::cout << "  3. 退出游戏\n";
	std::cout << "  4. 调试控制台（仅开发者）\n";
    std::cout << "\n";

    std::string input = prompt("请选择操作: ", theme.prompt);
    if (input == "1") {
        Game::getInstance().changeState(GameStateType::TEAM);
    } else if (input == "2") {
        Game::getInstance().changeState(GameStateType::ADVENTURE);
    } else if (input == "3") {
        Game::getInstance().changeState(GameStateType::EXIT);
	} else if (input == "4") {
		Game::getInstance().changeState(GameStateType::CONSOLE);
    } else {
        std::cout << adaptive_textcolor(theme.warning) << "无效输入，请重新选择。\n" << resetcolor();
    }
}