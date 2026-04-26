// adventure_state.cpp
#include "adventure_state.h"
#include "../customio.h"
#include "../entity.h"
#include "../console.h"   // for get_fast_score
#include "../act.h"       // SkillRegistry, make_unique<Attack> etc.
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <algorithm>
#include <memory>

using namespace customio;

// 战斗结算报表函数
void print_fight_report(Team& teamA, Team& teamB) {
    const auto& theme = get_console_theme();
    std::cout << "\n" << adaptive_textcolor(theme.title) << bold()
              << "========== 战斗结算 ==========" << resetcolor() << "\n";

    auto print_team = [](const std::string& team_name, const std::vector<std::reference_wrapper<character>>& members) {
        const auto& theme = get_console_theme();
        std::cout << adaptive_textcolor(theme.info) << "[" << team_name << "]" << resetcolor() << "\n";
        std::cout << std::setw(20) << std::left << "名称"
                  << std::setw(8) << "伤害" << std::setw(8) << "承伤"
                  << std::setw(6) << "击杀" << std::setw(8) << "治疗"
                  << std::setw(8) << "评分\n";
        std::cout << std::string(70, '-') << "\n";

        for (auto& ref : members) {
            character& ch = ref.get();
            double score = get_fast_score(ch.get_name());
            char grade = (score >= 85) ? 'S' : (score >= 70) ? 'A' : (score >= 55) ? 'B' : (score >= 40) ? 'C' : 'D';
            std::cout << std::setw(20) << std::left << ch.get_name()
                      << std::setw(8) << ch.damage_dealt
                      << std::setw(8) << ch.damage_taken
                      << std::setw(6) << ch.kills
                      << std::setw(8) << ch.healing_done
                      << std::setw(6) << grade << " (" << std::fixed << std::setprecision(0) << score << ")"
                      << "  ";
            std::cout << "\n";
        }
        std::cout << std::string(70, '-') << "\n";
    };

    print_team(teamA.get_name(), teamA.get_members());
    print_team(teamB.get_name(), teamB.get_members());

    // 头衔计算
    std::vector<character*> all_chars;
    for (auto& ref : teamA.get_members()) all_chars.push_back(&ref.get());
    for (auto& ref : teamB.get_members()) all_chars.push_back(&ref.get());

    if (!all_chars.empty()) {
        auto max_dmg = std::max_element(all_chars.begin(), all_chars.end(),
            [](character* a, character* b) { return a->damage_dealt < b->damage_dealt; });
        auto max_tank = std::max_element(all_chars.begin(), all_chars.end(),
            [](character* a, character* b) { return a->damage_taken < b->damage_taken; });
        auto max_heal = std::max_element(all_chars.begin(), all_chars.end(),
            [](character* a, character* b) { return a->healing_done < b->healing_done; });
        auto max_kills = std::max_element(all_chars.begin(), all_chars.end(),
            [](character* a, character* b) { return a->kills < b->kills; });

        if ((*max_dmg)->damage_dealt > 0)
            std::cout << "★ MVP（最高伤害）: " << (*max_dmg)->get_name() << "\n";
        if ((*max_tank)->damage_taken > 0)
            std::cout << "★ 金牌坦克（最高承伤）: " << (*max_tank)->get_name() << "\n";
        if ((*max_heal)->healing_done > 0)
            std::cout << "★ 神医（最高治疗）: " << (*max_heal)->get_name() << "\n";
        if ((*max_kills)->kills > 0)
            std::cout << "★ 人头王（最多击杀）: " << (*max_kills)->get_name() << "\n";
    }
    std::cout << std::endl;
}

AdventureState::AdventureState(GameContext& ctx) : GameState(ctx) {}

void AdventureState::on_enter() {
    // 由 update 控制
}

void AdventureState::show_level_list_menu() {
    size_t level_count = ctx_.level_manager.get_level_count();
    if (level_count == 0) {
        std::cout << "没有可用关卡。\n";
        std::cout << "按任意键继续...";
        _getch();
        return;
    }

    std::vector<std::string> menu_items;
    for (size_t i = 0; i < level_count; ++i) {
        const LevelData* lv = ctx_.level_manager.get_level(i);
        if (!lv) continue;
        std::string status;
        if (lv->completed) status = "[已完成]";
        else if (!lv->unlocked) status = "[未解锁]";
        else status = "[可挑战]";
        std::string item = std::to_string(i + 1) + ". " + lv->name + " " + status
            + " (敌方:" + std::to_string(lv->enemies.size())
            + " 上限:" + std::to_string(lv->max_allies) + ")";
        menu_items.push_back(item);
    }
    menu_items.push_back("返回大厅");
    menu_items.push_back("重置所有关卡 (调试)");

    int choice = menu_select(menu_items, "===== 冒险模式 - 选择关卡 =====");
    if (choice == -1 || choice == (int)menu_items.size() - 2) { // 返回大厅
        Game::getInstance().changeState(GameStateType::LOBBY);
        return;
    }
    if (choice == (int)menu_items.size() - 1) { // 重置关卡
        ctx_.level_manager = LevelManager();
        ctx_.level_manager.set_preset_manager(&ctx_.preset_manager);
        ctx_.level_manager.load_from_json("levels.json");
        return;
    }
    if (choice >= 0 && choice < (int)level_count) {
        const LevelData* lv = ctx_.level_manager.get_level(choice);
        if (!lv || !lv->unlocked) {
            std::cout << "该关卡尚未解锁！\n";
            game_sleep(1000);
            return;
        }
        if (ctx_.selected_team.empty()) {
            std::cout << "请先选择出战角色！\n";
            game_sleep(1000);
            return;
        }
        if ((int)ctx_.selected_team.size() > lv->max_allies) {
            if (!adjust_team_for_level(lv->max_allies)) {
                return;
            }
        }
        // 战斗 (play_level 内部已完成报表输出)
        bool victory = play_level(choice);
        if (victory) {
            ctx_.level_manager.mark_completed(choice);
            ctx_.level_manager.unlock_next(choice);
            std::cout << adaptive_textcolor(get_console_theme().heal) << bold()
                << "\n★ 关卡胜利！ ★\n" << resetcolor();
            game_sleep(1000);
        }
        else {
            std::cout << adaptive_textcolor(get_console_theme().damage) << bold()
                << "\n※ 战斗失败... ※\n" << resetcolor();
            game_sleep(1000);
        }
    }
}

bool AdventureState::adjust_team_for_level(int max_allies) {
    SelectionList selector;
    selector.set_item_count(ctx_.all_characters.size());
    selector.set_max_selection(max_allies);
    selector.set_page_size(10);
    for (size_t i = 0; i < std::min(ctx_.selected_team.size(), (size_t)max_allies); ++i) {
        size_t idx = ctx_.selected_team[i];
        if (idx < ctx_.all_characters.size()) {
            selector.set_selected(idx, true);
        }
    }
    auto render_func = [this](size_t index, bool selected, bool highlighted) {
        const auto& theme = get_console_theme();
        auto& ch = ctx_.all_characters[index];
        if (highlighted) {
            std::cout << background(theme.prompt) << adaptive_textcolor(theme.background);
        }
        std::cout << (selected ? "[*]" : "[ ]") << " "
            << std::setw(3) << index + 1 << ". "
            << std::setw(20) << std::left << ch->get_name()
            << " HP:" << std::setw(4) << ch->get_attribute("HP")
            << " ATK:" << std::setw(3) << ch->get_attribute("ATK")
            << " MATK:" << std::setw(3) << ch->get_attribute("MATK");
        if (highlighted) {
            std::cout << resetcolor();
        }
    };
    std::cout << adaptive_textcolor(get_console_theme().title) << bold()
        << "===== 调整出战角色 (最多 " << max_allies << " 人) =====" << resetcolor() << std::endl;
    std::vector<size_t> new_selected = selector.run(render_func);
    if (new_selected.empty()) {
        std::cout << "未选择任何角色，返回。\n";
        game_sleep(1000);
        return false;
    }
    ctx_.selected_team = std::move(new_selected);
    return true;
}

void AdventureState::update() {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 冒险模式 =====" << resetcolor() << std::endl;
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
    show_level_list_menu();
}

bool AdventureState::play_level(int level_index) {
    const LevelData* lv = ctx_.level_manager.get_level(level_index);
    if (!lv) return false;

    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
              << "===== " << lv->name << " =====" << resetcolor() << std::endl;
    std::cout << lv->description << "\n\n";

    // 构建我方队伍（克隆出战角色）
    Team player_team("冒险者");
    std::vector<std::unique_ptr<character>> cloned_characters;
    for (size_t idx : ctx_.selected_team) {
        if (idx >= ctx_.all_characters.size()) continue;
        character* original = ctx_.all_characters[idx].get();
        auto clone = std::make_unique<character>();
        clone->set_name(original->get_name());

        std::vector<std::string> attrs = {
            "MAX_HP", "HP", "MAX_MP", "MP", "ATK", "DEF", "MATK", "SPD",
            "CRIT", "CRIT_D", "C_AP"
        };
        for (const auto& attr : attrs) {
            int val = original->get_attribute(attr);
            if (attr == "HP") val = original->get_attribute("MAX_HP");
            if (attr == "MP") val = original->get_attribute("MAX_MP");
            clone->setattribute(attr, val);
        }
        clone->SetRule(BATTLE_WITHOUT_OUTPUT, original->GetRule(BATTLE_WITHOUT_OUTPUT));
        clone->SetRule(SHOW_ATTRIBUTES, original->GetRule(SHOW_ATTRIBUTES));
        clone->SetRule(SLOW_BATTLE, original->GetRule(SLOW_BATTLE));
        // 复制技能
        clone->get_actions().clear();
        for (const auto& action : original->get_actions()) {
            const SkillInfo* info = SkillRegistry::getSkillInfo(action->get_name());
            if (info) {
                clone->get_actions().push_back(info->factory());
            }
        }
        if (clone->get_actions().empty()) {
            clone->get_actions().push_back(std::make_unique<Attack>());
        }
        // 重置统计
        clone->reset_stats();
        player_team.add_character(*clone);
        cloned_characters.push_back(std::move(clone));
    }

    // 构建敌方队伍（使用预设或默认名）
    Team enemy_team("敌军");
    std::vector<std::unique_ptr<character>> enemy_chars;
    for (const auto& spawn : lv->enemies) {
        std::unique_ptr<character> enemy;
        if (!spawn.preset_id.empty() && ctx_.preset_manager.has_preset(spawn.preset_id)) {
            const MonsterPreset* preset = ctx_.preset_manager.get_preset(spawn.preset_id);
            enemy = std::make_unique<character>();
            enemy->set_name(preset->display_name.empty() ? spawn.preset_id : preset->display_name);
            for (const auto& attr : preset->attributes) {
                enemy->setattribute(attr.first, attr.second);
            }
            enemy->get_actions().clear();
            for (const auto& action_id : preset->actions) {
                const SkillInfo* info = SkillRegistry::getSkillInfo(action_id);
                if (info) {
                    enemy->get_actions().push_back(info->factory());
                }
            }
            if (enemy->get_actions().empty()) {
                enemy->get_actions().push_back(std::make_unique<Attack>());
            }
        }
        else {
            // 兼容旧格式：名字生成随机属性
            enemy = std::make_unique<character>(spawn.name.empty() ? "未知敌人" : spawn.name);
        }
        // 覆盖属性（如果关卡中有额外指定）
        for (const auto& attr : spawn.attributes) {
            enemy->setattribute(attr.first, attr.second);
        }
        // 确保关键属性存在
        if (enemy->get_attribute("MAX_HP") == 0) {
            enemy->setattribute("MAX_HP", enemy->get_attribute("HP"));
        }
        if (enemy->get_attribute("MAX_MP") == 0) {
            enemy->setattribute("MAX_MP", enemy->get_attribute("MP"));
        }
        enemy->SetRule(BATTLE_WITHOUT_OUTPUT, false);
        enemy->reset_stats();
        enemy_team.add_character(*enemy);
        enemy_chars.push_back(std::move(enemy));
    }

    // 战斗
    FightComponent fight;
    fight.add_team(player_team);
    fight.add_team(enemy_team);
    fight.start();

    bool victory = player_team.is_alive() && !enemy_team.is_alive();

    // 输出结算报表
    print_fight_report(player_team, enemy_team);

    std::cout << "\n按任意键继续...";
    _getch();
    return victory;
}