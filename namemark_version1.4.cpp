// namemark_version1.4.cpp
#include "entity.h"
#include "customio.h"
#include "act.h"
#include "game.h"        // 新增
#include <windows.h>

using namespace customio;

int main() {
    SetConsoleTitleA("Namemark");
    //system("mode con cols=120 lines=40"); 失去缓冲区还是太可怕了
    init_console();
    registerAllSkills();
    // 启动游戏
    Game::getInstance().run();
    return 0;
}