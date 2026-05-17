// console.h — 调试控制台与战斗评分系统
#pragma once
#include "entity.h"
#include <string>
#include <vector>
#include <iostream>

// 1vN 战斗结果
struct BattleResult {
    bool win;
    int damage_dealt;
    int damage_taken;
};

// 角色基准测试报告（1v1/1v2/1v3 各 N 场，综合评分 0-100）
struct BenchmarkReport {
    std::string char_name;
    double win_rate_1v1;
    double win_rate_1v2;
    double win_rate_1v3;
    double final_score;
    std::string grade;
    int static_score;
    void output() { std::cout << final_score; }
};

// 批量模拟战斗汇总
struct SimulateResult {
    int hero_wins = 0;
    int monster_wins = 0;
    int draws = 0;
    double hero_win_rate = 0.0;
};

// 调试控制台入口
void debug_console();

// 快速评分（基于静态属性，映射到 0-100）
double get_fast_score(const character& c);
double get_fast_score(const std::string& char_name);

// 对已有角色做完整战斗评分（10 场基准测试，结果缓存于角色对象）
std::pair<double, std::string> evaluate_real_character(character& c, int battles_per_test = 10);
void ensure_benchmark_cached(character& c);

// 工具
std::string trim(const std::string& s);
std::string random_string(int min_len, int max_len);

// 1vN 战斗：1 个英雄对 N 个标准敌人
BattleResult run_1vN_battle(const std::string& char_name, int enemy_count);

// 角色基准测试：30 场 1v1 / 1v2 / 1v3
BenchmarkReport evaluate_character_benchmark(const std::string& char_name, int battles_per_test = 30);
std::pair<double, std::string> evaluate_score(const std::string& char_name, int battles_per_test = 30);

// 批量模拟：英雄列表 vs 怪物列表，逗号分隔
std::vector<std::string> parse_name_list(const std::string& str);
SimulateResult run_simulation(const std::vector<std::string>& hero_names,
    const std::vector<std::string>& monster_names,
    int battle_count, bool show_fightlog);
