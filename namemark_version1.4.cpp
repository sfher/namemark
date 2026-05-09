// namemark_version1.4.cpp
#include "entity.h"
#include "customio.h"
#include "console.h"
#include "act.h"
#include "game.h"    
#ifdef _WIN32
#include <windows.h>
#endif

using namespace customio;

int main() {
#ifdef _WIN32
    SetConsoleTitleA("Namemark");
#endif
    init_console();
    registerAllSkills();
    // 启动游戏
    Game::getInstance().run();
    return 0;
}