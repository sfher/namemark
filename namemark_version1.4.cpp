#include "entity.h"
#include "customio.h"
#include <iostream>

using namespace customio;

int main() {
    init_console();

    std::string name = prompt("请输入你的名字：", color::magenta);

    // 创建角色（使用名字构造函数，自动随机生成属性）
    character hero(name);
    character assist1("战士");
    character assist2("法师");
    character assist3("你妈");
    // 创建怪物（使用默认构造函数，随机生成属性）
    character goblin("哥布林");
    character giga("巨魔");

    // 可以手动调整属性以便更快看到效果（可选）

    // 创建队伍
    Team playerTeam("英雄队伍");
    playerTeam.add_character(hero);
    playerTeam.add_character(assist1);
    playerTeam.add_character(assist2);
    playerTeam.add_character(assist3);

    Team enemyTeam("怪物队伍");
    enemyTeam.add_character(goblin);
    enemyTeam.add_character(giga);

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