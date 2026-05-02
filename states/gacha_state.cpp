// states/gacha_state.cpp
#include "gacha_state.h"
#include "../customio.h"
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <random>
#include <algorithm>

using namespace customio;

GachaState::GachaState(GameContext& ctx) : GameState(ctx) {}

PullResult GachaState::random_pull() {
    static std::mt19937& rng = get_random_engine();
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double roll = dist(rng);

    std::string selected_rarity = "R";
    double cumulative = 0.0;
    for (const auto& [rarity, rate] : ctx_.gacha_rates) {
        cumulative += rate;
        if (roll <= cumulative) {
            selected_rarity = rarity;
            break;
        }
    }

    auto& pool = ctx_.gacha_pools[selected_rarity];
    if (pool.empty()) return { "character", "flyhead", "R" };

    std::uniform_int_distribution<size_t> item_dist(0, pool.size() - 1);
    size_t idx = item_dist(rng);
    auto& [type, id] = pool[idx];
    return { type, id, selected_rarity };
}

void GachaState::single_pull() {
    if (ctx_.gold < ctx_.gacha_single_cost) {
        std::cout << "金币不足！\n按任意键返回...";
        _getch();
        return;
    }
    ctx_.gold -= ctx_.gacha_single_cost;
    auto result = random_pull();
    display_result({ result });
}

void GachaState::ten_pull() {
    if (ctx_.gold < ctx_.gacha_ten_cost) {
        std::cout << "金币不足！\n按任意键返回...";
        _getch();
        return;
    }
    ctx_.gold -= ctx_.gacha_ten_cost;
    std::vector<PullResult> results;
    for (int i = 0; i < 10; ++i) {
        results.push_back(random_pull());
    }
    display_result(results);
}


void GachaState::display_result(const std::vector<PullResult>& results) {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 抽卡结果 =====" << resetcolor() << "\n\n";

    for (const auto& [type, id, rarity] : results) {
        if (type == "character") {
            const MonsterPreset* preset = ctx_.preset_manager.get_preset(id);
            if (preset) {
                auto ch = std::make_unique<character>();
                ch->set_name(preset->display_name.empty() ? id : preset->display_name);
                for (const auto& attr : preset->attributes) {
                    ch->setattribute(attr.first, attr.second);
                }
                ch->get_actions().clear();
                for (const auto& action_id : preset->actions) {
                    const SkillInfo* info = SkillRegistry::getSkillInfo(action_id);
                    if (info) ch->get_actions().push_back(info->factory());
                }
                if (ch->get_actions().empty()) {
                    ch->get_actions().push_back(std::make_unique<Attack>());
                }
                ctx_.all_characters.push_back(std::move(ch));
                // 显示：稀有度 + 名字
                std::cout << "[" << rarity << "] " << preset->display_name << " (角色)\n";
            }
            else {
                std::cout << "[" << rarity << "] 未知角色 (" << id << ")\n";
            }
        }


        else if (type == "weapon") {
            // 在武器模板库中查找
            const WeaponData* template_weapon = nullptr;
            for (const auto& w : ctx_.weapon_templates) {
                if (w.id == id) {
                    template_weapon = &w;
                    break;
                }
            }
            if (template_weapon) {
                WeaponData new_weapon = *template_weapon;
                new_weapon.equipped = false;
                ctx_.weapons.push_back(new_weapon);
                std::cout << "[" << rarity << "] " << new_weapon.name << " (武器)\n";
            }
            else {
                std::cout << "[" << rarity << "] 未知武器 (" << id << ")\n";
            }
        }


        customio::game_sleep(300); // 逐行显示
    }

    std::cout << "\n剩余金币: " << ctx_.gold << "\n";
    std::cout << "按任意键继续...";
    _getch();
}

void GachaState::update() {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 抽卡 =====" << resetcolor() << "\n\n";
    std::cout << "金币: " << ctx_.gold << "\n";
    std::cout << "单抽: " << ctx_.gacha_single_cost << " 金币\n";
    std::cout << "十连: " << ctx_.gacha_ten_cost << " 金币\n\n";

    std::cout << "概率公示:\n";
    for (const auto& [rarity, rate] : ctx_.gacha_rates) {
        std::cout << "  " << rarity << ": " << std::fixed << std::setprecision(1) << rate * 100 << "%\n";
    }
    std::cout << "\n";

    std::vector<std::string> items = {
        "单抽",
        "十连抽",
        "返回大厅"
    };
    int choice = menu_select(items);
    switch (choice) {
    case 0: single_pull(); break;
    case 1: ten_pull(); break;
    case 2: Game::getInstance().changeState(GameStateType::LOBBY); break;
    }
}