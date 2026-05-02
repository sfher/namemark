// states/shop_state.cpp
#include "shop_state.h"
#include "../customio.h"
#include "../console.h"
#include "../act.h"
#include "../weapon_data.h"
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <algorithm>

using namespace customio;

ShopState::ShopState(GameContext& ctx) : GameState(ctx) {}

void ShopState::on_enter() {}

void ShopState::show_weapon_list() {
    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 商店 =====" << resetcolor() << "\n\n";

    // 检查武器模板库，而不是玩家武器库
    if (ctx_.weapon_templates.empty()) {
        std::cout << "当前模组没有武器可购买。\n按任意键返回...";
        _getch();
        return;
    }

    std::cout << "你的金币: " << ctx_.gold << "\n\n";

    // 遍历武器模板库
    std::vector<std::string> items;
    for (const auto& w : ctx_.weapon_templates) {
        std::string item = w.name + " [" + w.rarity + "] - " + std::to_string(w.price) + " 金币";
        if (!w.description.empty()) {
            item += "\n    " + w.description;
        }
        if (!w.attributes.empty()) {
            item += "\n    属性: ";
            for (const auto& [attr, val] : w.attributes) {
                item += attr + "+" + std::to_string(val) + " ";
            }
        }
        if (!w.skill_grant.empty()) {
            item += "\n    技能授予: " + w.skill_grant;
        }
        items.push_back(item);
    }
    items.push_back("返回");

    int choice = menu_select(items, "选择武器购买 (Enter确认)");
    if (choice < 0 || choice == (int)items.size() - 1) {
        return;
    }

    // 购买选中的武器（传入模板库中的引用，但仅用于显示，实际在 buy_weapon 中创建副本）
    const WeaponData& selected_template = ctx_.weapon_templates[choice];
    buy_weapon(selected_template);
}

void ShopState::buy_weapon(const WeaponData& weapon_template) {
    if (ctx_.gold < weapon_template.price) {
        std::cout << "金币不足！\n按任意键返回...";
        _getch();
        return;
    }
    show_character_select_for_weapon(weapon_template);
}

void ShopState::show_character_select_for_weapon(const WeaponData& weapon_template) {
    if (ctx_.all_characters.empty()) {
        std::cout << "没有可装备的角色，请先创建角色。\n按任意键返回...";
        _getch();
        return;
    }

    const auto& theme = get_console_theme();
    clear_screen();
    std::cout << adaptive_textcolor(theme.title) << bold()
        << "===== 选择装备角色 =====" << resetcolor() << "\n\n";
    std::cout << "即将购买: " << weapon_template.name << " (" << weapon_template.price << " 金币)\n\n";

    std::vector<std::string> items;
    for (size_t i = 0; i < ctx_.all_characters.size(); ++i) {
        auto& ch = ctx_.all_characters[i];
        std::string item = ch->get_name();
        if (ch->has_weapon()) {
            const WeaponData* current = ch->get_equipped_weapon(ctx_.weapons);
            if (current) {
                item += " [已装备: " + current->name + "]";
            }
        }
        items.push_back(item);
    }
    items.push_back("取消");

    int choice = menu_select(items, "为谁购买武器？");
    if (choice < 0 || choice == (int)items.size() - 1) return;

    // 从模板创建新的武器实物
    WeaponData new_weapon = weapon_template;
    new_weapon.equipped = false;
    ctx_.weapons.push_back(new_weapon);
    WeaponData& actual_weapon = ctx_.weapons.back();

    // 装备新武器
    character* target = ctx_.all_characters[choice].get();
    bool ok = target->equip_weapon(actual_weapon);
    if (ok) {
        ctx_.gold -= weapon_template.price;
        std::cout << "\n购买成功！" << target->get_name() << " 装备了 " << new_weapon.name << "。\n";
        std::cout << "剩余金币: " << ctx_.gold << "\n按任意键继续...";
    }
    else {
        // 装备失败，移除刚添加的武器
        ctx_.weapons.pop_back();
        std::cout << "装备失败！\n按任意键返回...";
    }
    _getch();
}

void ShopState::update() {
    show_weapon_list();
    Game::getInstance().changeState(GameStateType::LOBBY);
}