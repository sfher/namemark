#include "entity.h"
#include "customio.h"
#include "act.h"
#include "console.h" 
using namespace customio;
// ... 其他头文件

int main() {
	init_console(); 
    registerAllSkills();
    debug_console(); 
    return 0;
}