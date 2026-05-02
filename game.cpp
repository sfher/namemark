// game.cpp
#include "game.h"
#include "states/lobby_state.h"
#include "states/team_state.h"
#include "states/adventure_state.h"
#include "states/setting_state.h"
#include "states/teamtest_state.h"
#include "states/shop_state.h"
#include "states/gacha_state.h"
#include "skill_loader.h"
#include "weapon_loader.h"
#include "generic_skill.h"
#include "json.hpp"
#include "customio.h"
#include "console.h"
#include <iostream>
#include <fstream>
#include <conio.h>

using namespace customio;

Game::Game() {
    ctx_.preset_manager.load_from_json("monsters.json");
    ctx_.level_manager.set_preset_manager(&ctx_.preset_manager);
    ctx_.level_manager.load_from_json("levels.json");
}

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

void Game::changeState(GameStateType type) {
    if (current_state_) {
        current_state_->on_exit();
        current_state_.reset();
    }
    switch (type) {
    case GameStateType::LOBBY:
        current_state_ = std::make_unique<LobbyState>(ctx_);
        break;
    case GameStateType::TEAM:
        current_state_ = std::make_unique<TeamState>(ctx_);
        break;
    case GameStateType::TEAM_TEST:
        current_state_ = std::make_unique<TeamTestState>(ctx_);
        break;
    case GameStateType::GACHA:
        current_state_ = std::make_unique<GachaState>(ctx_);
        break;
    case GameStateType::ADVENTURE:
        current_state_ = std::make_unique<AdventureState>(ctx_);
        break;
    case GameStateType::SHOP:
        current_state_ = std::make_unique<ShopState>(ctx_);
        break;
    case GameStateType::CONSOLE:
        debug_console();
        break;
    case GameStateType::SETTINGS:
        current_state_ = std::make_unique<SettingsState>(ctx_);
        break;
    case GameStateType::EXIT:
        running_ = false;
        return;
    }
    current_type_ = type;
    if (current_state_) {
        current_state_->on_enter();
    }
}

void Game::run() {
    // 1. 扫描模组
    auto packages = ctx_.package_manager.scan();
    if (packages.empty()) {
        std::cout << "未找到任何模组包！请检查 packages/ 目录。\n";
        return;
    }

    // 2. 构建菜单
    std::vector<std::string> items;
    for (const auto& pkg : packages) {
        items.push_back(pkg.display);
    }
    items.push_back("退出");

    // 3. 用户选择
    int choice = menu_select(items, "===== 选择游戏模组 =====");


    if (choice < 0 || choice == (int)items.size() - 1) {
        return; // 退出
    }

    changeState(GameStateType::LOBBY);

    // 4. 加载所选模组
    const auto& selected = packages[choice];
    ctx_.package_manager.set_current(selected.name);
    
    std::string pkg_path = selected.path;
    
    // 清空旧技能，注册内置技能
    SkillRegistry::clearAll();
    registerAllSkills();  // 里面只注册派生类技能

   auto external_skills = SkillLoader::load_from_directory(pkg_path + "/skills/");
    for (const auto& data : external_skills) {
    SkillInfo info(
        data.display_name,
        data.acquire_chance,
        data.weight,
        [data]() -> std::unique_ptr<act> {
            return std::make_unique<GenericSkill>(data);
        }
    );

    info.random_available = data.random_available;
    
    SkillRegistry::registerSkill(data.id, info);
    }
    // 加载武器
    auto templates = WeaponLoader::load_from_file(pkg_path + "/weapons.json");
    ctx_.weapon_templates = std::move(templates);
    ctx_.weapons.clear(); // 玩家武器库初始为空
    std::cout << "加载了 " << ctx_.weapon_templates.size() << " 种武器模板\n";

    // 3. 加载模组的怪物和关卡（此时已拥有完整的技能表）
    bool ok = true;
    ok &= ctx_.level_manager.load_from_json(pkg_path + "/levels.json");
    ok &= ctx_.preset_manager.load_from_json(pkg_path + "/monsters.json");
    ctx_.level_manager.set_preset_manager(&ctx_.preset_manager);

    if (!ok) {
        std::cout << "模组加载失败，请检查文件。\n按任意键退出...";
        _getch();
        return;
    }

    std::ifstream file;
    file.open(pkg_path + "/gacha.json");
      if (file.is_open()) {
            nlohmann::json gacha_json;
			gacha_json = nlohmann::json::parse(file);
            ctx_.gacha_single_cost = gacha_json.value("single_cost", 300);
            ctx_.gacha_ten_cost = gacha_json.value("ten_cost", 2800);
            for (auto& [rarity, pool] : gacha_json["pools"].items()) {
                ctx_.gacha_rates[rarity] = pool["rate"];
                for (const auto& item : pool["items"]) {
                    std::string type = item["type"];
                    std::string id = (type == "character") ? item["preset_id"] : item["weapon_id"];
                    ctx_.gacha_pools[rarity].push_back({ type, id });
                }
            }
        }

    //  进入主循环
    changeState(GameStateType::LOBBY);
    while (running_ && current_state_) {
        clear_screen();
        current_state_->update();
        if (!running_) break;
    }
}