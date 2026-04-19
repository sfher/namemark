// states/team_state.cpp
#include "team_state.h"
#include "../customio.h"
#include "../console.h"    // for get_fast_score
#include <iostream>
#include <iomanip>

using namespace customio;

TeamState::TeamState(GameContext& ctx) : GameState(ctx) {}

void TeamState::on_enter() {
    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 队伍管理 =====" << resetcolor() << std::endl;
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

    // 配置选择列表
    selection_list_ = std::make_unique<SelectionList>();
    selection_list_->set_item_count(ctx_.all_characters.size());
    selection_list_->set_max_selection(4); // 暂定上限 4 人
    selection_list_->set_page_size(10);

    // 预置已选中的索引（根据 ctx_.selected_team 初始化）
    for (size_t idx : ctx_.selected_team) {
        if (idx < ctx_.all_characters.size()) {
            selection_list_->set_selected(idx, true);
        }
    }

    // 渲染回调
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

        // 高亮行使用不同颜色
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

    // 输出标题
    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 选择出战角色 (最多 4 人) =====" << resetcolor() << std::endl;

    // 运行选择
    std::vector<size_t> new_selected = selection_list_->run(render_func);

    // 更新上下文中的出战队伍
    ctx_.selected_team = std::move(new_selected);

    std::cout << "\n出战队伍已更新！当前出战角色: " << ctx_.selected_team.size() << " 人\n";
}

void TeamState::update() {
    const auto& theme = get_console_theme();
    list_characters();

    std::cout << "\n";
    std::cout << "  1. 添加新角色\n";
    std::cout << "  2. 选择出战队伍\n";
    std::cout << "  3. 返回大厅\n";
    std::cout << "\n";

    std::string input = prompt("请选择操作: ", theme.prompt);
    if (input == "1") {
        add_new_character();
    }
    else if (input == "2") {
        select_team();
    }
    else if (input == "3") {
        Game::getInstance().changeState(GameStateType::LOBBY);
    }
    else {
        std::cout << adaptive_textcolor(theme.warning) << "无效输入。\n" << resetcolor();
    }
}