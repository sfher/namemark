// cli.h — 命令行模式入口
// 当 namemark 带参数启动时，解析一级命令并调度到对应功能（复用控制台逻辑）
#pragma once

// 解析 argc/argv，执行对应命令后返回退出码
int cli_mode(int argc, char* argv[]);
void print_cli_help();
