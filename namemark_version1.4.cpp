// namemark_version1.4.cpp
#include "entity.h"
#include "customio.h"
#include "console.h"
#include "act.h"
#include "game.h"    
#include <windows.h>

using namespace customio;

int main() {
    SetConsoleTitleA("Namemark");
    init_console();
    registerAllSkills();
    // 启动游戏
    Game::getInstance().run();
    return 0;
}