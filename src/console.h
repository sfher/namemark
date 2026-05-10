// console.h
#pragma once
#include "entity.h"   // 必须，这里 character 可用
#include <string>
#include <vector>

struct BattleResult;
struct BenchmarkReport;
struct SimulateResult;

void debug_console();
double get_fast_score(const character& c);
double get_fast_score(const std::string& char_name);
std::pair<double, std::string> evaluate_real_character(character& c, int battles_per_test = 10);
void ensure_benchmark_cached(character& c);
std::string trim(const std::string& s);
std::string random_string(int min_len, int max_len);
BattleResult run_1vN_battle(const std::string& char_name, int enemy_count);
BenchmarkReport evaluate_character_benchmark(const std::string& char_name, int battles_per_test = 30);
std::pair<double, std::string> evaluate_score(const std::string& char_name, int battles_per_test = 30);
std::vector<std::string> parse_name_list(const std::string& str);
SimulateResult run_simulation(const std::vector<std::string>& hero_names,
    const std::vector<std::string>& monster_names,
    int battle_count, bool show_fightlog);