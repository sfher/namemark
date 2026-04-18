#include "entity.h"
#include "customio.h"
#include "act.h"
#include "console.h" 
#include "damage_calculator.h"
#include "skill_executor.h"
#include <windows.h>
using namespace customio;
// ... 其他头文件

void test() {
    
}

int main() {
    //修改控制台标题
    SetConsoleTitleA("Namemark");
    system("mode con cols=90 lines=90"); // 设置控制台大小
	init_console(); 
    registerAllSkills();
    debug_console();
    return 0;
}
