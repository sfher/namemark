// namemark_version1.4.cpp
#include "entity.h"
#include "customio.h"
#include "console.h"
#include "cli.h"
#include "act.h"
#include "game.h"
#include <csignal>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace customio;

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

static void restore_terminal() {
    std::cout << "\033[0m" << std::flush;    // reset all attributes
#ifndef _WIN32
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}

#ifdef _WIN32
static BOOL WINAPI ctrl_handler(DWORD fdwCtrlType) {
    (void)fdwCtrlType;
    restore_terminal();
    std::cout << "\nNamemark terminated.\n";
    ExitProcess(0);
    return TRUE;
}
#else
static void sigint_handler(int /*signum*/) {
    restore_terminal();
    std::cout << "\nNamemark terminated.\n" << std::flush;
    _exit(0);
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleTitleA("Namemark");
    SetConsoleCtrlHandler(ctrl_handler, TRUE);
#else
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);
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