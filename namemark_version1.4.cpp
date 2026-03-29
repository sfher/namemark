#include "entity.h"
#include "customio.h"
#include <iostream>

using namespace customio;

int main() {
    init_console();

    // 创建角色（使用名字构造函数，自动随机生成属性）
    character hero("勇者");
    character goblin("哥布林");

    // 可以手动调整属性以便更快看到效果（可选）
    hero.setattribute("HP", 2200);
    hero.setattribute("ATK", 40);
    hero.setattribute("DEF", 15);
    hero.setattribute("SPD", 25);
    hero.setattribute("CRIT", 20);
    hero.setattribute("CRIT_D", 180);
    hero.setattribute("C_AP", 100);

    goblin.setattribute("HP", 1200);
    goblin.setattribute("ATK", 25);
    goblin.setattribute("DEF", 8);
    goblin.setattribute("SPD", 18);
    goblin.setattribute("CRIT", 10);
    goblin.setattribute("CRIT_D", 150);
    goblin.setattribute("C_AP", 100);

    // 创建队伍
    Team playerTeam("英雄队伍");
    playerTeam.add_character(hero);

    Team enemyTeam("怪物队伍");
    enemyTeam.add_character(goblin);

    // 输出队伍信息
    std::cout << bold() << textcolor(color::cyan) << "=== 队伍信息 ===" << resetcolor() << std::endl;
    playerTeam.output_team_info();
    enemyTeam.output_team_info();

    // 开始战斗
    FightComponent fight;
    fight.add_team(playerTeam);
    fight.add_team(enemyTeam);

    std::cout << bold() << textcolor(color::blue) << "\n===== 战斗开始 =====" << resetcolor() << std::endl;
    fight.start();

    std::cout << bold() << textcolor(color::magenta) << "\n===== 战斗结束 =====" << resetcolor() << std::endl;
    std::cout << "英雄队伍存活: " << (playerTeam.is_alive() ? "是" : "否") << std::endl;
    std::cout << "怪物队伍存活: " << (enemyTeam.is_alive() ? "是" : "否") << std::endl;

    return 0;
}