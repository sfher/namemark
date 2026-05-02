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
        double score = get_fast_score(*ch);
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
    selection_list_->set_max_selection(5);
    selection_list_->set_page_size(10);

    for (size_t idx : ctx_.selected_team) {
        if (idx < ctx_.all_characters.size()) {
            selection_list_->set_selected(idx, true);
        }
    }

    auto render_func = [this](size_t index, bool selected, bool highlighted) {
        const auto& theme = get_console_theme();
        auto& ch = ctx_.all_characters[index];
        double score = get_fast_score(*ch);
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
        << "===== 选择出战角色 (最多 5 人) =====" << resetcolor() << std::endl;
    std::vector<size_t> new_selected = selection_list_->run(render_func);
    ctx_.selected_team = std::move(new_selected);
    std::cout << "\n出战队伍已更新！当前出战角色: " << ctx_.selected_team.size() << " 人\n";
    std::cout << "按任意键继续...";
    char ch = _getch();
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
                  << "===== 查看角色详情 (↑↓选择, Enter返回, D战斗评分, E装备/卸下武器) =====" << resetcolor() << "\n\n";

        // 角色列表
        for (size_t i = 0; i < ctx_.all_characters.size(); ++i) {
            auto& ch = ctx_.all_characters[i];
            double score = get_fast_score(*ch);
            char grade = (score >= 85) ? 'S' : (score >= 70) ? 'A' : (score >= 55) ? 'B' : (score >= 40) ? 'C' : 'D';

            std::string marker = (i == cursor) ? ">" : " ";
            std::cout << marker << std::setw(3) << i + 1 << ". "
                      << std::setw(20) << std::left << ch->get_name()
                      << " HP:" << std::setw(4) << ch->get_attribute("HP")
                      << " ATK:" << std::setw(3) << ch->get_attribute("ATK")
                      << " MATK:" << std::setw(3) << ch->get_attribute("MATK")
                      << " 评分:" << grade << "\n";
        }

        character& ch = *ctx_.all_characters[cursor];
        std::cout << "\n--- " << ch.get_name() << " ---\n\n";

        // 属性
        std::cout << adaptive_textcolor(theme.info) << "【属性】" << resetcolor() << "\n";
        for (const auto& attr : {"HP", "MAX_HP", "MP", "MAX_MP", "ATK", "DEF", "MATK", "SPD", "CRIT", "CRIT_D", "C_AP"}) {
            std::cout << std::setw(8) << attr << ": " << ch.get_attribute(attr) << "\n";
        }

        // 加护
        if (ch.has_aegis()) {
            std::cout << "\n" << adaptive_textcolor(color::magenta) << "【加护】" << resetcolor() << "\n";
            for (const auto& aeg : ch.get_aegis()) {
                std::cout << "  " << aeg << "\n";
            }
        }

        // 技能
        const auto& actions = ch.get_actions();
        std::cout << "\n" << adaptive_textcolor(theme.info) << "【技能】" << resetcolor() << "\n";
        for (size_t i = 0; i < actions.size(); ++i) {
            std::cout << "  " << i + 1 << ". " << actions[i]->get_name();
            if (!actions[i]->get_description().empty())
                std::cout << " - " << actions[i]->get_description();
            std::cout << "\n";
        }

        // 战斗评分（缓存）
        std::cout << "\n" << adaptive_textcolor(theme.info) << "【战斗评分】" << resetcolor() << "\n";
        if (ch.is_benchmark_cached()) {
            double bscore = ch.get_benchmark_score();
            std::string bgrade = ch.get_benchmark_grade();
            std::cout << "  " << bgrade << " (" << std::fixed << std::setprecision(1) << bscore << ")\n";
        } else {
            std::cout << "  未测试 (按 D 键进行评分)\n";
        }

        // 武器
        std::cout << "\n" << adaptive_textcolor(theme.info) << "【武器】" << resetcolor() << "\n";
        const WeaponData* weapon = ch.get_equipped_weapon(ctx_.weapons);
        if (weapon) {
            std::cout << "  " << weapon->name << " (" << weapon->rarity << ")";
            if (!weapon->description.empty()) std::cout << " - " << weapon->description;
            std::cout << "\n  属性: ";
            for (const auto& [attr, val] : weapon->attributes) {
                std::cout << attr << "+" << val << " ";
            }
            if (!weapon->skill_grant.empty())
                std::cout << "\n  技能: " << weapon->skill_grant;
            std::cout << "\n";
        } else {
            std::cout << "  未装备\n";
        }

        // 操作提示
        std::cout << "\n-----------------------------------\n";
        std::cout << "↑↓: 选择  D: 战斗评分  E: ";
        if (ch.has_weapon())
            std::cout << "卸下武器  Enter: 返回\n";
        else
            std::cout << "装备武器  Enter: 返回\n";
    };

    draw();

    while (running) {
        int ch = _getch();
        if (ch == 224) { // 方向键
            ch = _getch();
            if (ch == 72 && cursor > 0) cursor--;
            else if (ch == 80 && cursor < ctx_.all_characters.size() - 1) cursor++;
            draw();
        }
        else if (ch == 'e' || ch == 'E') {
            character& current = *ctx_.all_characters[cursor];
            if (current.has_weapon()) {
                std::string removed_id = current.unequip_weapon();
                for (auto& w : ctx_.weapons) {
                    if (w.id == removed_id) {
                        w.equipped = false;
                        break;
                    }
                }
                std::cout << "\n武器已卸下！按任意键...";
                _getch();
            }
            else {
                std::vector<std::string> weapon_items;
                std::vector<size_t> weapon_indices;
                for (size_t i = 0; i < ctx_.weapons.size(); ++i) {
                    auto& w = ctx_.weapons[i];
                    if (w.equipped) continue;
                    std::string label = w.name + " (" + w.rarity + ")";
                    if (!w.description.empty()) label += " - " + w.description;
                    weapon_items.push_back(label);
                    weapon_indices.push_back(i);
                }
                if (weapon_items.empty()) {
                    std::cout << "\n没有可装备的武器。按任意键...";
                    _getch();
                }
                else {
                    weapon_items.push_back("取消");
                    int wchoice = menu_select(weapon_items, "选择要装备的武器:");
                    if (wchoice >= 0 && wchoice < (int)weapon_indices.size()) {
                        WeaponData& selected = ctx_.weapons[weapon_indices[wchoice]];
                        bool ok = current.equip_weapon(selected);
                        if (ok) {
                            selected.equipped = true;
                            std::cout << "\n装备成功！按任意键...";
                        }
                        else {
                            std::cout << "\n装备失败！按任意键...";
                        }
                        _getch();
                    }
                }
            }
            draw();
        }
        else if (ch == 'd' || ch == 'D') {
            character& current = *ctx_.all_characters[cursor];
            if (!current.is_benchmark_cached()) {
                std::cout << "\n正在计算战斗评分 (10场测试)，请稍候...\n";
                ensure_benchmark_cached(current);
                // 输出一下
                double bscore = current.get_benchmark_score();
                std::string bgrade = current.get_benchmark_grade();
                std::cout << "评分完成！ " << bgrade << " (" << std::fixed << std::setprecision(1) << bscore << ")\n";
                std::cout << "详细评分数据已缓存，下次查看角色详情时将直接显示。\n";
                std::cout << "按任意键继续...";
                _getch();
            }
            draw();
        }
        else if (ch == 13 || ch == 27) { // Enter 或 Esc
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
        "召唤新角色",
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