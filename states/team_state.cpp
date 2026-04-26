// states/team_state.cpp
#include "team_state.h"
#include "../customio.h"
#include "../console.h"
#include <iostream>
#include <iomanip>

using namespace customio;

TeamState::TeamState(GameContext& ctx) : GameState(ctx) {}

void TeamState::on_enter() {
    // 标题由 update 负责
}

void TeamState::add_new_character() {
    const auto& theme = get_console_theme();
    std::string name = prompt("请输入新角色名字 (留空随机): ", theme.prompt);
    if (name.empty()) {
        name = "冒险者" + std::to_string(rand() % 1000);
    }
    auto ch = std::make_unique<character>(name);
    ch->SetRule(SHOW_ATTRIBUTES, false);
    ctx_.all_characters.push_back(std::move(ch));
    std::cout << adaptive_textcolor(theme.info) << "OK！ " << name << " 加入了队伍！\n" << resetcolor();
    std::cout << "按任意键继续...";
    _getch();
}

void TeamState::list_characters() {
    if (ctx_.all_characters.empty()) {
        std::cout << "当前没有任何角色。\n";
        return;
    }
    std::cout << "\n--- 已拥有角色 (" << ctx_.all_characters.size() << " 位) ---\n";
    for (size_t i = 0; i < ctx_.all_characters.size(); ++i) {
        auto& ch = ctx_.all_characters[i];
        double score = get_fast_score(ch->get_name());
        char grade = '?';
        if (score >= 85) grade = 'S';
        else if (score >= 70) grade = 'A';
        else if (score >= 55) grade = 'B';
        else if (score >= 40) grade = 'C';
        else grade = 'D';
        std::cout << std::setw(3) << i + 1 << ". "
            << std::setw(20) << std::left << ch->get_name()
            << " HP:" << std::setw(4) << ch->get_attribute("HP")
            << " ATK:" << std::setw(3) << ch->get_attribute("ATK")
            << " MATK:" << std::setw(3) << ch->get_attribute("MATK")
            << " 评分:" << grade << "(" << std::fixed << std::setprecision(0) << score << ")"
            << std::endl;
    }
}

void TeamState::select_team() {
    if (ctx_.all_characters.empty()) {
        std::cout << "请先添加至少一个角色！\n";
        return;
    }
    selection_list_ = std::make_unique<SelectionList>();
    selection_list_->set_item_count(ctx_.all_characters.size());
    selection_list_->set_max_selection(4);
    selection_list_->set_page_size(10);

    for (size_t idx : ctx_.selected_team) {
        if (idx < ctx_.all_characters.size()) {
            selection_list_->set_selected(idx, true);
        }
    }

    auto render_func = [this](size_t index, bool selected, bool highlighted) {
        const auto& theme = get_console_theme();
        auto& ch = ctx_.all_characters[index];
        double score = get_fast_score(ch->get_name());
        char grade = '?';
        if (score >= 85) grade = 'S';
        else if (score >= 70) grade = 'A';
        else if (score >= 55) grade = 'B';
        else if (score >= 40) grade = 'C';
        else grade = 'D';
        if (highlighted) {
            std::cout << background(theme.prompt) << adaptive_textcolor(theme.background);
        }
        std::cout << (selected ? "[*]" : "[ ]") << " "
            << std::setw(3) << index + 1 << ". "
            << std::setw(20) << std::left << ch->get_name()
            << " HP:" << std::setw(4) << ch->get_attribute("HP")
            << " ATK:" << std::setw(3) << ch->get_attribute("ATK")
            << " MATK:" << std::setw(3) << ch->get_attribute("MATK")
            << " 评分:" << grade;
        if (highlighted) {
            std::cout << resetcolor();
        }
        };

    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 选择出战角色 (最多 4 人) =====" << resetcolor() << std::endl;
    std::vector<size_t> new_selected = selection_list_->run(render_func);
    ctx_.selected_team = std::move(new_selected);
    std::cout << "\n出战队伍已更新！当前出战角色: " << ctx_.selected_team.size() << " 人\n";
    std::cout << "按任意键继续...";
    _getch();
}

void TeamState::view_character_detail() {
    if (ctx_.all_characters.empty()) {
        std::cout << "当前没有任何角色。\n按任意键返回...";
        _getch();
        return;
    }

    const auto& theme = get_console_theme();
    size_t cursor = 0;
    bool running = true;

    auto draw = [&]() {
        clear_screen();
        std::cout << adaptive_textcolor(theme.title) << bold()
                  << "===== 查看角色详情 (↑↓选择, Enter返回) =====" << resetcolor() << "\n\n";

        // 左侧角色列表
        for (size_t i = 0; i < ctx_.all_characters.size(); ++i) {
            auto& ch = ctx_.all_characters[i];
            double score = get_fast_score(ch->get_name());
            char grade = (score >= 85) ? 'S' : (score >= 70) ? 'A' : (score >= 55) ? 'B' : (score >= 40) ? 'C' : 'D';

            std::string marker = (i == cursor) ? ">" : " ";
            std::cout << marker << std::setw(3) << i + 1 << ". "
                      << std::setw(20) << std::left << ch->get_name()
                      << " HP:" << std::setw(4) << ch->get_attribute("HP")
                      << " ATK:" << std::setw(3) << ch->get_attribute("ATK")
                      << " MATK:" << std::setw(3) << ch->get_attribute("MATK")
                      << " 评分:" << grade << "\n";
        }

        // 右侧详情（当前高亮角色）
        character& ch = *ctx_.all_characters[cursor];
        std::cout << "\n--- " << ch.get_name() << " 的详细信息 ---\n\n";

        // 属性表
        std::cout << adaptive_textcolor(theme.info) << "【属性】" << resetcolor() << "\n";
        std::vector<std::string> attrs = {"HP", "MAX_HP", "MP", "MAX_MP", "ATK", "DEF", "MATK", "SPD", "CRIT", "CRIT_D", "C_AP"};
        for (const auto& attr : attrs) {
            std::cout << std::setw(8) << attr << ": " << ch.get_attribute(attr) << "\n";
        }

        // 加护
        if (ch.has_aegis()) {
            std::cout << "\n" << adaptive_textcolor(color::magenta) << "【加护】" << resetcolor() << "\n";
            for (const auto& aeg : ch.get_aegis()) {
                std::cout << "  " << aeg << "\n";
            }
        }

        // 技能列表
        const auto& actions = ch.get_actions();
        std::cout << "\n" << adaptive_textcolor(theme.info) << "【技能】" << resetcolor() << "\n";
        for (size_t i = 0; i < actions.size(); ++i) {
            std::string name = actions[i]->get_name();
            std::string desc = actions[i]->get_description();
            std::cout << "  " << i + 1 << ". " << name;
            if (!desc.empty()) {
                std::cout << " - " << desc;
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    };

    draw();
    while (running) {
        int ch = _getch();
        if (ch == 224) {          // 方向键
            ch = _getch();
            if (ch == 72 && cursor > 0) cursor--;
            else if (ch == 80 && cursor < ctx_.all_characters.size() - 1) cursor++;
            draw();
        } else if (ch == 13) {    // 回车返回
            running = false;
        }
    }
}

void TeamState::update() {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
              << "===== 队伍管理 =====" << resetcolor() << std::endl;
    list_characters();

    std::cout << "\n";
    std::vector<std::string> items = {
        "添加新角色",
        "选择出战队伍",
        "查看角色详情",
        "返回大厅"
    };
    int choice = menu_select(items);
    switch (choice) {
        case 0: add_new_character(); break;
        case 1: select_team(); break;
        case 2: view_character_detail(); break;
        case 3: Game::getInstance().changeState(GameStateType::LOBBY); break;
        default: break;
    }
}