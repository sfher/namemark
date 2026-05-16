// namemark_version1.4.cpp
#include "entity.h"
#include "customio.h"
#include "console.h"
#include "cli.h"
#include "act.h"
#include "game.h"
#ifdef _WIN32
#include <windows.h>
#endif

using namespace customio;

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleTitleA("Namemark");
#endif
    init_console();
    registerAllSkills();

    // CLI mode when arguments are provided
    if (argc > 1)
        return cli_mode(argc, argv);

    // Normal interactive game
    Game::getInstance().run();
    return 0;
}