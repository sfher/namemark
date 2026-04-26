// states/settings_state.cpp
#include "setting_state.h"
#include "../customio.h"
#include <iostream>
#include <vector>

using namespace customio;

SettingsState::SettingsState(GameContext& ctx) : GameState(ctx) {}

void SettingsState::on_enter() {
    // 标题由 update 负责显示
}

void SettingsState::update() {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
              << Utf8ToAnsi("===== 设置 =====") << resetcolor() << "\n\n";

    // 显示当前速度 (g_battle_speed 是在 customio 中定义的全局变量)
    std::cout << Utf8ToAnsi("当前战斗速度: ") << g_battle_speed << "x\n\n";

    // 注意所有字符串都使用 Utf8ToAnsi 转码，避免设置界面乱码
    std::vector<std::string> speed_items = {
        Utf8ToAnsi("0.5x"),
        Utf8ToAnsi("1.0x"),
        Utf8ToAnsi("1.5x"),
        Utf8ToAnsi("2.0x"),
        Utf8ToAnsi("返回")
    };

    int choice = menu_select(speed_items, Utf8ToAnsi("选择战斗速度:"));
    switch (choice) {
        case 0: g_battle_speed = 0.5f; break;
        case 1: g_battle_speed = 1.0f; break;
        case 2: g_battle_speed = 1.5f; break;
        case 3: g_battle_speed = 2.0f; break;
        case 4: Game::getInstance().changeState(GameStateType::LOBBY); return;
        default: break;
    }
    // 设置完后可以留在此界面，也可直接返回大厅，这里选择留在设置界面
}