// adventure_state.cpp
#define NOMINMAX
#include <windows.h>

#include "adventure_state.h"
#include "../customio.h"
#include "../entity.h"
#include <iostream>
#include <iomanip>
#include <conio.h>

using namespace customio;

AdventureState::AdventureState(GameContext& ctx) : GameState(ctx) {}

void AdventureState::on_enter() {
    // 不自动清屏，由 update 控制
}

void AdventureState::update() {
    const auto& theme = get_console_theme();
    clear_screen();

    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 冒险模式 =====" << resetcolor() << std::endl;

    // 显示当前出战队伍
    std::cout << "当前出战队伍 (";
    if (ctx_.selected_team.empty()) {
        std::cout << adaptive_textcolor(theme.warning) << "未选择" << resetcolor();
    }
    else {
        std::cout << ctx_.selected_team.size() << " 人";
    }
    std::cout << "): ";
    for (size_t idx : ctx_.selected_team) {
        if (idx < ctx_.all_characters.size()) {
            std::cout << ctx_.all_characters[idx]->get_name() << " ";
        }
    }
    std::cout << "\n\n";

    // 显示关卡列表
    size_t level_count = ctx_.level_manager.get_level_count();
    if (level_count == 0) {
        std::cout << "没有可用关卡，请检查 levels.json。\n";
    }
    else {
        std::cout << "--- 关卡列表 ---\n";
        for (size_t i = 0; i < level_count; ++i) {
            const LevelData* lv = ctx_.level_manager.get_level(i);
            if (!lv) continue;

            std::string status;
            if (lv->completed) {
                status = "[已完成]";
            }
            else if (!lv->unlocked) {
                status = "[未解锁]";
            }
            else {
                status = "[可挑战]";
            }

            // 根据状态设置颜色
            color status_color = theme.text;
            if (lv->completed) status_color = color::green;
            else if (!lv->unlocked) status_color = color::yellow;
            else status_color = color::cyan;

            std::cout << std::setw(3) << i + 1 << ". "
                << std::setw(20) << std::left << lv->name
                << " " << adaptive_textcolor(status_color) << status << resetcolor()
                << " (敌方:" << lv->enemies.size() << " 上限:" << lv->max_allies << ")\n";
            std::cout << "    " << lv->description << "\n";
        }
    }

    std::cout << "\n-----------------------------------\n";
    std::cout << "  输入关卡编号挑战 (已解锁且未完成)\n";
    std::cout << "  输入 0 返回大厅\n";
    std::cout << "  输入 r 重置所有关卡状态 (调试)\n";

    std::string input = prompt("请选择: ", theme.prompt);
    if (input == "0") {
        Game::getInstance().changeState(GameStateType::LOBBY);
        return;
    }
    else if (input == "r" || input == "R") {
        // 调试功能：重置关卡
        ctx_.level_manager = LevelManager();
        ctx_.level_manager.load_from_json("levels.json");
        return;
    }

    try {
        int choice = std::stoi(input) - 1;
        if (choice < 0 || choice >= (int)level_count) {
            std::cout << adaptive_textcolor(theme.warning) << "无效的关卡编号。\n" << resetcolor();
            Sleep(1000);
            return;
        }

        const LevelData* lv = ctx_.level_manager.get_level(choice);
        if (!lv->unlocked) {
            std::cout << adaptive_textcolor(theme.warning) << "该关卡尚未解锁！\n" << resetcolor();
            Sleep(1000);
            return;
        }
        if (lv->completed) {
            std::cout << adaptive_textcolor(theme.info) << "该关卡已完成，是否重新挑战？(y/n): " << resetcolor();
            char ch = _getch();
            if (ch != 'y' && ch != 'Y') {
                return;
            }
        }

        // 检查出战人数
        if (ctx_.selected_team.size() > (size_t)lv->max_allies) {
            std::cout << adaptive_textcolor(theme.warning)
                << "出战人数 (" << ctx_.selected_team.size() << ") 超过关卡限制 (" << lv->max_allies << ")！\n"
                << "请返回队伍界面调整。\n" << resetcolor();
            Sleep(2000);
            return;
        }
        if (ctx_.selected_team.empty()) {
            std::cout << adaptive_textcolor(theme.warning) << "请先选择出战角色！\n" << resetcolor();
            Sleep(1000);
            return;
        }

        // 开始战斗
        bool victory = play_level(choice);
        if (victory) {
            ctx_.level_manager.mark_completed(choice);
            ctx_.level_manager.unlock_next(choice);
            std::cout << adaptive_textcolor(theme.heal) << bold()
                << "\n★ 关卡胜利！ ★\n" << resetcolor();
            Sleep(1500);
        }
        else {
            std::cout << adaptive_textcolor(theme.damage) << bold()
                << "\n※ 战斗失败... ※\n" << resetcolor();
            Sleep(1500);
        }
    }
    catch (...) {
        std::cout << adaptive_textcolor(theme.warning) << "无效输入。\n" << resetcolor();
        Sleep(1000);
    }
}

bool AdventureState::play_level(int level_index) {
    const LevelData* lv = ctx_.level_manager.get_level(level_index);
    if (!lv) return false;

    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== " << lv->name << " =====" << resetcolor() << std::endl;
    std::cout << lv->description << "\n\n";

    // 构建我方队伍 (克隆出战角色，保证原角色状态不变)
    Team player_team("冒险者");
    std::vector<std::unique_ptr<character>> cloned_characters; // 管理克隆角色的生命周期

    for (size_t idx : ctx_.selected_team) {
        if (idx >= ctx_.all_characters.size()) continue;
        character* original = ctx_.all_characters[idx].get();

        // 创建克隆角色
        auto clone = std::make_unique<character>();
        clone->set_name(original->get_name());

        // 复制关键属性，确保满状态
        std::vector<std::string> attrs = {
            "MAX_HP", "HP", "MAX_MP", "MP", "ATK", "DEF", "MATK", "SPD",
            "CRIT", "CRIT_D", "C_AP"
        };
        for (const auto& attr : attrs) {
            int val = original->get_attribute(attr);
            // 对于 HP 和 MP，设为最大值
            if (attr == "HP") val = original->get_attribute("MAX_HP");
            if (attr == "MP") val = original->get_attribute("MAX_MP");
            clone->setattribute(attr, val);
        }

        // 复制规则
        clone->SetRule(BATTLE_WITHOUT_OUTPUT, original->GetRule(BATTLE_WITHOUT_OUTPUT));
        clone->SetRule(SHOW_ATTRIBUTES, original->GetRule(SHOW_ATTRIBUTES));
        clone->SetRule(SLOW_BATTLE, original->GetRule(SLOW_BATTLE));

        // 复制技能列表（通过工厂重新创建）
        clone->get_actions().clear();
        for (const auto& action : original->get_actions()) {
            const SkillInfo* info = SkillRegistry::getSkillInfo(action->get_name());
            if (info) {
                clone->get_actions().push_back(info->factory());
            }
            else {
                // 如果找不到对应技能，至少给个默认攻击
                clone->get_actions().push_back(std::make_unique<Attack>());
            }
        }

        // 加入队伍
        player_team.add_character(*clone);
        cloned_characters.push_back(std::move(clone));
    }

    // 构建敌方队伍
    Team enemy_team("敌军");
    std::vector<std::unique_ptr<character>> enemy_chars;
    for (const auto& spawn : lv->enemies) {
        std::unique_ptr<character> enemy;

        if (!spawn.preset_id.empty() && ctx_.preset_manager.has_preset(spawn.preset_id)) {
            // 从预设创建
            const MonsterPreset* preset = ctx_.preset_manager.get_preset(spawn.preset_id);
            enemy = std::make_unique<character>();
            enemy->set_name(preset->display_name.empty() ? spawn.preset_id : preset->display_name);

            // 应用预设属性
            for (const auto& attr : preset->attributes) {
                enemy->setattribute(attr.first, attr.second);
            }
            // 应用预设技能
            enemy->get_actions().clear();
            for (const auto& action_id : preset->actions) {
                const SkillInfo* info = SkillRegistry::getSkillInfo(action_id);
                if (info) {
                    enemy->get_actions().push_back(info->factory());
                }
            }
            // 如果预设没有技能，给个默认攻击
            if (enemy->get_actions().empty()) {
                enemy->get_actions().push_back(std::make_unique<Attack>());
            }
        }
        else {
            // 兼容旧方式：直接通过名字生成随机属性
            enemy = std::make_unique<character>(spawn.name.empty() ? "未知敌人" : spawn.name);
        }

        // 应用覆盖属性（无论是预设还是直接生成的）
        for (const auto& attr : spawn.attributes) {
            enemy->setattribute(attr.first, attr.second);
        }

        // 确保关键属性存在（如果预设中没有定义MAX_HP等，补充默认值）
        if (enemy->get_attribute("MAX_HP") == 0) {
            enemy->setattribute("MAX_HP", enemy->get_attribute("HP"));
        }
        if (enemy->get_attribute("MAX_MP") == 0) {
            enemy->setattribute("MAX_MP", enemy->get_attribute("MP"));
        }

        enemy->SetRule(BATTLE_WITHOUT_OUTPUT, false);
        enemy_team.add_character(*enemy);
        enemy_chars.push_back(std::move(enemy));
    }

    // 战斗
    FightComponent fight;
    fight.add_team(player_team);
    fight.add_team(enemy_team);
    fight.start();

    bool victory = player_team.is_alive() && !enemy_team.is_alive();

    // 战斗结束，可选择是否保留详细日志
    std::cout << "\n按任意键继续...";
    _getch();
    return victory;
}