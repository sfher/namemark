// states/teamtest_state.cpp
#include "teamtest_state.h"
#include "../customio.h"
#include "../console.h"
#include "../act.h"
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <algorithm>

using namespace customio;

TeamTestState::TeamTestState(GameContext& ctx) : GameState(ctx) {}
void TeamTestState::on_enter() {}

void TeamTestState::select_team(const std::string& title, size_t max_selection,
                                std::vector<size_t>& result) {
    if (ctx_.all_characters.empty()) {
        std::cout << "没有可用的角色。\n";
        _getch();
        return;
    }

    selector_ = std::make_unique<SelectionList>();
    selector_->set_item_count(ctx_.all_characters.size());
    selector_->set_max_selection(max_selection);
    selector_->set_page_size(10);

    for (size_t idx : result) {
        if (idx < ctx_.all_characters.size())
            selector_->set_selected(idx, true);
    }

    auto render_func = [this](size_t index, bool selected, bool highlighted) {
        const auto& theme = get_console_theme();
        auto& ch = ctx_.all_characters[index];
        double score = get_fast_score(*ch);
        char grade = (score >= 85) ? 'S' : (score >= 70) ? 'A' : (score >= 55) ? 'B' : (score >= 40) ? 'C' : 'D';
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
        if (highlighted) std::cout << resetcolor();
    };

    std::cout << adaptive_textcolor(get_console_theme().title) << bold()
              << "===== " << title << " (最多 " << max_selection << " 人) =====" << resetcolor() << std::endl;
    result = selector_->run(render_func);
}

// 保存单个英雄的战前状态
struct HeroSnapshot {
    character* hero;
    int hp, mp, cap;
    std::unordered_map<std::string, std::pair<int, int>> buffs;
};

static void save_snapshots(const std::vector<character*>& heroes,
                           std::vector<HeroSnapshot>& snapshots) {
    for (auto hero : heroes) {
        HeroSnapshot snap;
        snap.hero = hero;
        snap.hp = hero->get_attribute("HP");
        snap.mp = hero->get_attribute("MP");
        snap.cap = hero->get_attribute("C_AP");
        // 拷贝buff状态
        snap.buffs = hero->get_buffs();
        snapshots.push_back(snap);
        hero->reset_stats();
    }
}

static void restore_snapshots(const std::vector<HeroSnapshot>& snapshots) {
    for (const auto& snap : snapshots) {
        character* hero = snap.hero;
        hero->setattribute("HP", snap.hp);
        hero->setattribute("MP", snap.mp);
        hero->setattribute("C_AP", snap.cap);
        hero->set_buffs(snap.buffs);
        hero->reset_stats();
    }
}

void TeamTestState::start_exercise() {
    std::vector<size_t> teamA_idx, teamB_idx;
    select_team("选择队伍 A", 5, teamA_idx);
    if (teamA_idx.empty()) return;
    select_team("选择队伍 B", 5, teamB_idx);
    if (teamB_idx.empty()) return;

    // 检查重复角色
    for (size_t a : teamA_idx) {
        for (size_t b : teamB_idx) {
            if (a == b) {
                std::cout << "队伍 A 和队伍 B 不能包含同一个角色！\n按任意键返回...";
                _getch();
                return;
            }
        }
    }

    // 准备本体参战
    std::vector<character*> teamA_heroes, teamB_heroes;
    Team teamA("队伍 A"), teamB("队伍 B");

    for (size_t idx : teamA_idx) {
        character* hero = ctx_.all_characters[idx].get();
        hero->SetRule(BATTLE_WITHOUT_OUTPUT, false);
        teamA.add_character(*hero);
        teamA_heroes.push_back(hero);
    }
    for (size_t idx : teamB_idx) {
        character* hero = ctx_.all_characters[idx].get();
        hero->SetRule(BATTLE_WITHOUT_OUTPUT, false);
        teamB.add_character(*hero);
        teamB_heroes.push_back(hero);
    }

    // 保存快照
    std::vector<HeroSnapshot> snapshots;
    save_snapshots(teamA_heroes, snapshots);
    save_snapshots(teamB_heroes, snapshots);

    // 战斗
    FightComponent fight;
    fight.add_team(teamA);
    fight.add_team(teamB);
    fight.start();

    print_exercise_report(teamA, teamB);

    // 恢复本体
    restore_snapshots(snapshots);

    std::cout << "\n按任意键继续...";
    _getch();
}

void TeamTestState::print_exercise_report(Team& teamA, Team& teamB) {
    const auto& theme = get_console_theme();
    std::cout << "\n" << adaptive_textcolor(theme.title) << bold()
              << "========== 演习结算 ==========" << resetcolor() << "\n";

    auto print_team = [](const std::string& name, const std::vector<std::reference_wrapper<character>>& members) {
        std::cout << adaptive_textcolor(get_console_theme().info) << "[" << name << "]" << resetcolor() << "\n";
        printf("%-22s %-8s %-8s %-6s %-8s %-8s\n", "名称", "伤害", "承伤", "击杀", "治疗", "评分");
        std::cout << std::string(62, '-') << std::endl;
        for (const auto& ref : members) {
            const character& ch = ref.get();
            double score = get_fast_score(ch);
            char grade = (score >= 85) ? 'S' : (score >= 70) ? 'A' : (score >= 55) ? 'B' : (score >= 40) ? 'C' : 'D';
            printf("%-22s %-8d %-8d %-6d %-8d %c (%.0f)\n",
                   ch.get_name().c_str(), ch.damage_dealt, ch.damage_taken, ch.kills, ch.healing_done, grade, score);
        }
        std::cout << std::string(62, '-') << std::endl;
    };

    print_team(teamA.get_name(), teamA.get_members());
    print_team(teamB.get_name(), teamB.get_members());

    std::vector<character*> all;
    for (auto& ref : teamA.get_members()) all.push_back(&ref.get());
    for (auto& ref : teamB.get_members()) all.push_back(&ref.get());

    if (all.empty()) return;
    auto best_dmg   = *std::max_element(all.begin(), all.end(),
                        [](character* a, character* b) { return a->damage_dealt < b->damage_dealt; });
    auto best_tank  = *std::max_element(all.begin(), all.end(),
                        [](character* a, character* b) { return a->damage_taken < b->damage_taken; });
    auto best_heal  = *std::max_element(all.begin(), all.end(),
                        [](character* a, character* b) { return a->healing_done < b->healing_done; });
    auto best_kills = *std::max_element(all.begin(), all.end(),
                        [](character* a, character* b) { return a->kills < b->kills; });

    if (best_dmg->damage_dealt   > 0) std::cout << "★ MVP: "      << best_dmg->get_name()   << "\n";
    if (best_tank->damage_taken  > 0) std::cout << "★ 金牌坦克: " << best_tank->get_name()  << "\n";
    if (best_heal->healing_done  > 0) std::cout << "★ 神医: "     << best_heal->get_name()  << "\n";
    if (best_kills->kills        > 0) std::cout << "★ 人头王: "   << best_kills->get_name() << "\n";
}

void TeamTestState::update() {
    clear_screen();
    const auto& theme = get_console_theme();
    std::cout << adaptive_textcolor(theme.title) << bold()
              << "===== 队内演习 =====" << resetcolor() << "\n\n"
              << "选择两支队伍进行对战演习，结果不影响原角色。\n\n";

    if (ctx_.all_characters.size() < 2) {
        std::cout << "角色不足，至少需要2名角色。\n按任意键返回...";
        _getch();
        Game::getInstance().changeState(GameStateType::LOBBY);
        return;
    }

    std::vector<std::string> items = {
        "开始演习",
        "返回大厅"
    };
    if (menu_select(items) == 0) start_exercise();
    else Game::getInstance().changeState(GameStateType::LOBBY);
}