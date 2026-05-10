#include "entity.h"
#include "customio.h"
#include "act.h"
#include "damage_calculator.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>

using namespace customio;

// Forward declaration of helper functions
std::string trim(const std::string& s);

// ========== 随机字符串生成 ==========
std::string random_string(int min_len, int max_len) {
    static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::mt19937& rng = get_random_engine();
    std::uniform_int_distribution<int> len_dist(min_len, max_len);
    int len = len_dist(rng);
    std::uniform_int_distribution<int> char_dist(0, chars.size() - 1);
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; ++i) {
        result += chars[char_dist(rng)];
    }
    return result;
}

// ========== 全局调试存储 ==========
static std::map<std::string, std::unique_ptr<Team>> debug_teams;
static std::vector<std::unique_ptr<character>> debug_chars;

// ========== 通用数据结构 ==========
struct BattleResult {
    bool win;
    int damage_dealt;
    int damage_taken;
};

// ========== 静态属性评分 ==========
int calc_static_score(character& c) {
    int hp = c.get_attribute("HP");
    int atk = c.get_attribute("ATK");
    int def = c.get_attribute("DEF");
    int spd = c.get_attribute("SPD");
    int crit = c.get_attribute("CRIT");
    int crit_d = c.get_attribute("CRIT_D");
    int matk = c.get_attribute("MATK");

    double phys_dps = atk * (1 + crit / 100.0 * (crit_d / 100.0 - 1));
    double magic_dps = matk * (1 + crit / 100.0 * (crit_d / 100.0 - 1));
    double survival = hp * (1 + def / 100.0);
    double speed_factor = spd / 20.0;

    double score = (phys_dps + magic_dps) * speed_factor + survival * 0.5;
    return static_cast<int>(score);
}
// 快速评分（基于静态属性，映射到 0-100）
double get_fast_score(const std::string& char_name) {
    character tmp(char_name);
    tmp.SetRule(SHOW_ATTRIBUTES, false);
    int raw = calc_static_score(tmp);
    // 假设最高分 400，映射到 100
    double score = std::min(100.0, raw / 4.0);
    return score;
}
double get_fast_score(const character& c) {
    int raw = calc_static_score(const_cast<character&>(c)); // calc_static_score 接受非const引用，但实际只读，可以安全 const_cast
    return std::min(100.0, raw / 4.0);
}

// ========== 通用 1vN 战斗（N个标准敌人） ==========
BattleResult run_1vN_battle(const std::string& char_name, int enemy_count) {
    character hero(char_name);
    hero.SetRule(BATTLE_WITHOUT_OUTPUT, true);
    hero.SetRule(SHOW_ATTRIBUTES, false);
    int hero_initial_hp = hero.get_attribute("HP");

    Team monsterTeam("Monsters");
    std::vector<std::unique_ptr<character>> monsters;
    int monster_total_hp_start = 0;
    for (int i = 0; i < enemy_count; ++i) {
        auto mon = std::make_unique<character>("标准敌人");
        mon->setattribute("HP", 150);
        mon->setattribute("MAX_HP", 150);
        mon->setattribute("MP", 0);
        mon->setattribute("ATK", 40);
        mon->setattribute("MATK", 30);
        mon->setattribute("DEF", 20);
        mon->setattribute("SPD", 30);
        mon->setattribute("CRIT", 10);
        mon->setattribute("CRIT_D", 150);
        mon->setattribute("C_AP", 30);
        mon->init_default_actions(); //这很重要
        mon->SetRule(BATTLE_WITHOUT_OUTPUT, true);
        mon->SetRule(SHOW_ATTRIBUTES, false);

        monster_total_hp_start += mon->get_attribute("HP");
        monsterTeam.add_character(*mon);
        monsters.push_back(std::move(mon));
    }

    Team heroTeam("Hero");
    heroTeam.add_character(hero);

    FightComponent fight;
    fight.add_team(heroTeam);
    fight.add_team(monsterTeam);
    fight.start();

    bool win = hero.is_alive() && !monsterTeam.is_alive();
    int hero_damage_taken = hero_initial_hp - hero.get_attribute("HP");
    int monster_total_hp_end = 0;
    for (auto& mon : monsters) {
        monster_total_hp_end += mon->get_attribute("HP");
    }
    int total_damage_dealt = monster_total_hp_start - monster_total_hp_end;

    return { win, total_damage_dealt, hero_damage_taken };
}

// ========== 基准测试评分报告 ==========
struct BenchmarkReport {
    std::string char_name;
    double win_rate_1v1;   // 对1个标准敌人胜率
    double win_rate_1v2;   // 对2个标准敌人胜率
    double win_rate_1v3;   // 对3个标准敌人胜率
    double final_score;
    std::string grade;
    int static_score;
    void output() {
        std::cout << final_score;
    }
};

BenchmarkReport evaluate_character_benchmark(const std::string& char_name, int battles_per_test) {
    BenchmarkReport report = { char_name, 0,0,0,0,"",0 };

    int wins = 0;
    for (int i = 0; i < battles_per_test; ++i) {
        auto res = run_1vN_battle(char_name, 1);
        if (res.win) wins++;
    }
    report.win_rate_1v1 = (double)wins / battles_per_test * 100.0;

    wins = 0;
    for (int i = 0; i < battles_per_test; ++i) {
        auto res = run_1vN_battle(char_name, 2);
        if (res.win) wins++;
    }
    report.win_rate_1v2 = (double)wins / battles_per_test * 100.0;

    wins = 0;
    for (int i = 0; i < battles_per_test; ++i) {
        auto res = run_1vN_battle(char_name, 3);
        if (res.win) wins++;
    }
    report.win_rate_1v3 = (double)wins / battles_per_test * 100.0;

    report.final_score = report.win_rate_1v1 * 0.5 + report.win_rate_1v2 * 0.3 + report.win_rate_1v3 * 0.2;

    if (report.final_score >= 85) report.grade = "S";
    else if (report.final_score >= 70) report.grade = "A";
    else if (report.final_score >= 50) report.grade = "B";
    else if (report.final_score >= 30) report.grade = "C";
    else report.grade = "D";

    character tmp(char_name);
    tmp.SetRule(SHOW_ATTRIBUTES, false);
    report.static_score = calc_static_score(tmp);

    return report;
}

// ========== 评分函数 ==========
std::pair<double, std::string> evaluate_score(const std::string& char_name, int battles_per_test) {
    BenchmarkReport report = evaluate_character_benchmark(char_name, battles_per_test);
    return { report.final_score, report.grade };
}


// 对现有角色进行模拟战斗评分
#include <iostream>   // 确保已包含
#include <sstream>

std::pair<double, std::string> evaluate_real_character(character& c, int battles_per_test) {
    int old_hp   = c.get_attribute("HP");
    int old_mp   = c.get_attribute("MP");
    int old_cap  = c.get_attribute("C_AP");
    auto old_buffs = c.get_buffs();
    c.SetRule(BATTLE_WITHOUT_OUTPUT, true);

    int wins1 = 0, wins2 = 0, wins3 = 0;

    // ================= 1v1 =================
    for (int i = 0; i < battles_per_test; ++i) {
        auto mon = std::make_unique<character>("标准敌人");
        mon->setattribute("HP", 150); mon->setattribute("MAX_HP", 150);
        mon->setattribute("MP", 0);   mon->setattribute("ATK", 40);
        mon->setattribute("MATK", 30); mon->setattribute("DEF", 20);
        mon->setattribute("SPD", 30);  mon->setattribute("CRIT", 10);
        mon->setattribute("CRIT_D", 150); mon->setattribute("C_AP", 30);
        mon->init_default_actions();
        mon->SetRule(BATTLE_WITHOUT_OUTPUT, true);

        Team heroTeam("Hero");
        Team monsterTeam("Monster");
        heroTeam.add_character(c);
        monsterTeam.add_character(*mon);

        FightComponent fight;
        fight.add_team(heroTeam);
        fight.add_team(monsterTeam);
        fight.start();

        if (c.is_alive()) wins1++;

        // 恢复状态
        c.setattribute("HP", old_hp);
        c.setattribute("MP", old_mp);
        c.setattribute("C_AP", old_cap);
        c.set_buffs(old_buffs);
    }

    // ================= 1v2 =================
    for (int i = 0; i < battles_per_test; ++i) {
        Team heroTeam("Hero");
        Team monsterTeam("Monsters");
        std::vector<std::unique_ptr<character>> mobs;

        heroTeam.add_character(c);
        for (int j = 0; j < 2; ++j) {
            auto mon = std::make_unique<character>("标准敌人");
            mon->setattribute("HP", 150); mon->setattribute("MAX_HP", 150);
            mon->setattribute("MP", 0);   mon->setattribute("ATK", 40);
            mon->setattribute("MATK", 30); mon->setattribute("DEF", 20);
            mon->setattribute("SPD", 30);  mon->setattribute("CRIT", 10);
            mon->setattribute("CRIT_D", 150); mon->setattribute("C_AP", 30);
            mon->init_default_actions();
            mon->SetRule(BATTLE_WITHOUT_OUTPUT, true);
            monsterTeam.add_character(*mon);
            mobs.push_back(std::move(mon));
        }

        FightComponent fight;
        fight.add_team(heroTeam);
        fight.add_team(monsterTeam);
        fight.start();

        if (c.is_alive()) wins2++;

        c.setattribute("HP", old_hp);
        c.setattribute("MP", old_mp);
        c.setattribute("C_AP", old_cap);
        c.set_buffs(old_buffs);
    }

    // ================= 1v3 =================
    for (int i = 0; i < battles_per_test; ++i) {
        Team heroTeam("Hero");
        Team monsterTeam("Monsters");
        std::vector<std::unique_ptr<character>> mobs;

        heroTeam.add_character(c);
        for (int j = 0; j < 3; ++j) {
            auto mon = std::make_unique<character>("标准敌人");
            mon->setattribute("HP", 150); mon->setattribute("MAX_HP", 150);
            mon->setattribute("MP", 0);   mon->setattribute("ATK", 40);
            mon->setattribute("MATK", 30); mon->setattribute("DEF", 20);
            mon->setattribute("SPD", 30);  mon->setattribute("CRIT", 10);
            mon->setattribute("CRIT_D", 150); mon->setattribute("C_AP", 30);
            mon->init_default_actions();
            mon->SetRule(BATTLE_WITHOUT_OUTPUT, true);
            monsterTeam.add_character(*mon);
            mobs.push_back(std::move(mon));
        }

        FightComponent fight;
        fight.add_team(heroTeam);
        fight.add_team(monsterTeam);
        fight.start();

        if (c.is_alive()) wins3++;

        c.setattribute("HP", old_hp);
        c.setattribute("MP", old_mp);
        c.setattribute("C_AP", old_cap);
        c.set_buffs(old_buffs);
    }

    // 恢复可视设置
    c.SetRule(BATTLE_WITHOUT_OUTPUT, false);
    c.setattribute("HP", old_hp);
    c.setattribute("MP", old_mp);

    double win_rate_1v1 = (double)wins1 / battles_per_test * 100.0;
    double win_rate_1v2 = (double)wins2 / battles_per_test * 100.0;
    double win_rate_1v3 = (double)wins3 / battles_per_test * 100.0;
    double final_score = win_rate_1v1 * 0.5 + win_rate_1v2 * 0.3 + win_rate_1v3 * 0.2;

    std::string grade;
    if (final_score >= 85) grade = "S";
    else if (final_score >= 70) grade = "A";
    else if (final_score >= 55) grade = "B";
    else if (final_score >= 40) grade = "C";
    else grade = "D";

    return { final_score, grade };
}
// 缓存战斗评分：仅在未计算时运行
void ensure_benchmark_cached(character& c) {
    if (!c.is_benchmark_cached()) {   // 原先为 c.get_benchmark_score() < 0.0
        auto [score, grade] = evaluate_real_character(c, 10);
        c.set_benchmark_score(score);
        c.set_benchmark_grade(grade);
    }
}

// ========== 批量模拟战斗 ==========
struct SimulateResult {
    int hero_wins = 0;
    int monster_wins = 0;
    int draws = 0;
    double hero_win_rate = 0.0;
};

std::vector<std::string> parse_name_list(const std::string& str) {
    std::vector<std::string> names;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) names.push_back(item);
    }
    return names;
}

SimulateResult run_simulation(const std::vector<std::string>& hero_names,
    const std::vector<std::string>& monster_names,
    int battle_count, bool show_fightlog) {
    SimulateResult result;
    for (int i = 0; i < battle_count; ++i) {
        Team heroTeam("HeroTeam");
        std::vector<std::unique_ptr<character>> hero_chars;
        for (const auto& name : hero_names) {
            auto ch = std::make_unique<character>(name);
            if (!show_fightlog) ch->SetRule(BATTLE_WITHOUT_OUTPUT, true);
            if (!show_fightlog) ch->SetRule(SHOW_ATTRIBUTES, false);
            heroTeam.add_character(*ch);
            hero_chars.push_back(std::move(ch));
        }

        Team monsterTeam("MonsterTeam");
        std::vector<std::unique_ptr<character>> monster_chars;
        for (const auto& name : monster_names) {
            auto ch = std::make_unique<character>(name);
            ch->SetRule(BATTLE_WITHOUT_OUTPUT, true);
            ch->SetRule(SHOW_ATTRIBUTES, false);
            monsterTeam.add_character(*ch);
            monster_chars.push_back(std::move(ch));
        }

        FightComponent fight;
        fight.add_team(heroTeam);
        fight.add_team(monsterTeam);
        fight.start();

        bool hero_alive = heroTeam.is_alive();
        bool monster_alive = monsterTeam.is_alive();

        if (hero_alive && !monster_alive) result.hero_wins++;
        else if (!hero_alive && monster_alive) result.monster_wins++;
        else result.draws++;
    }
    result.hero_win_rate = (double)result.hero_wins / battle_count * 100.0;
    return result;
}

// ========== 辅助函数 ==========
std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) ++start;
    auto end = s.end();
    do { --end; } while (end != start && std::isspace(*end));
    return std::string(start, end + 1);
}

void output_help() {
    themed_println("Namemark Debug Console Help", color::yellow, true);
    themed_println("----------------------------------------", color::white);
    themed_println("Commands:", color::cyan, true);
    themed_println("  @<TeamName>            - Create a team (e.g. @HeroTeam)", color::white);
    themed_println("  <CharName> @<TeamName> - Create character and add to team", color::white);
    themed_println("  [:ID] @<TeamName>      - Use imported character template to create character", color::white);
    themed_println("  /help                  - Show help", color::white);
    themed_println("  /fight                 - Start battle (all teams)", color::white);
    themed_println("  /test <CharName>       - View character attributes", color::white);
    themed_println("  /ftest [CharName]      - Quick score (static attributes)", color::white);
    themed_println("  /import <filepath>     - Import characters from JSON file", color::white);
    themed_println("  /simulate <list> vs <list> [times] - Batch battle simulations", color::white);
    themed_println("  /find                  - Find characters with aegis", color::white);
    themed_println("  /theme [name]          - List or switch available themes", color::white);
    themed_println("  /settheme [name]       - Interactive theme selector or switch theme by name", color::white);
    themed_println("  /end                   - Exit debug console", color::white);
    themed_println("  /hfind                 - Find character with high benchmark score", color::white);
    themed_println("  /cmd [command]         - use cmd commands", color::white);
    themed_block("Tip: Character names cannot contain spaces, team names need @prefix, use [:ID] format for imported characters", color::white);
    themed_println("----------------------------------------", color::white);
}
// ========== Debug Console ==========
void debug_console() {
    set_theme_by_name("default"); // 默认使用 default 主题

    const auto& theme = customio::get_console_theme();
    std::cout << customio::background(theme.background)
              << customio::adaptive_textcolor(theme.title)
              << bold() << "\n===== Debug Console Started =====" << resetcolor() << std::endl;
    output_help();

    std::string line;
    std::string current_team_name;
    while (true) {
        const auto& theme = customio::get_console_theme();
        std::cout << customio::background(theme.background)
                  << customio::adaptive_textcolor(theme.prompt) << "debug> " << resetcolor();
        // 读取输入(让输入的颜色随着prompt颜色变化)
        line = customio::prompt("", theme.prompt);
        line = trim(line);
        if (line.empty()) continue;

        if (line == "/end") {
            std::cout << "Exiting debug console." << std::endl;
            break;
        }

        // Create team
        if (line[0] == '@') {
            std::string team_name = line.substr(1);
            if (debug_teams.find(team_name) != debug_teams.end()) {
                std::cout << "Team " << team_name << " already exists." << std::endl;
            }
            else {
                auto team = std::make_unique<Team>(team_name);
                team->set_name(team_name);
                debug_teams[team_name] = std::move(team);
                std::cout << "Team [" << team_name << "] created successfully." << std::endl;
            }
            current_team_name = team_name;
            continue;
        }

        // Create and add character to team (explicit team assignment)
        size_t at_pos = line.find('@');
        if (at_pos != std::string::npos) {
            std::string char_name = trim(line.substr(0, at_pos));
            std::string team_name = trim(line.substr(at_pos + 1));
            if (char_name.empty() || team_name.empty()) {
                std::cout << "Format error: use \"CharName @TeamName\"." << std::endl;
                continue;
            }
            auto it = debug_teams.find(team_name);
            if (it == debug_teams.end()) {
                std::cout << "Team " << team_name << " does not exist, create team first (@" << team_name << ")." << std::endl;
                continue;
            }
            auto ch = std::make_unique<character>(char_name);
            ch->SetRule(SHOW_ATTRIBUTES, false);
            it->second->add_character(*ch);
            debug_chars.push_back(std::move(ch));
            std::cout << "Character [" << char_name << "] added to team [" << team_name << "]." << std::endl;
            continue;
        }

        // If starts with normal character and not a command, add to current active team
        if (!line.empty() && line[0] != '/') {
            std::string char_name = line;
            if (current_team_name.empty()) {
                std::cout << "No active team, cannot add character. Create team first (e.g. @TeamName) or use \"CharName @TeamName\"." << std::endl;
                continue;
            }
            auto it = debug_teams.find(current_team_name);
            if (it == debug_teams.end()) {
                std::cout << "Current active team does not exist, recreate team (@" << current_team_name << ")." << std::endl;
                current_team_name.clear();
                continue;
            }
            auto ch = std::make_unique<character>(char_name);
            ch->SetRule(SHOW_ATTRIBUTES, false);
            it->second->add_character(*ch);
            debug_chars.push_back(std::move(ch));
            std::cout << "Character [" << char_name << "] added to team [" << current_team_name << "]." << std::endl;
            continue;
        }

        // Battle (using all currently created teams)
        if (line == "/fight") {
            if (debug_teams.empty()) {
                std::cout << "No teams, cannot start battle. Create team and add characters first." << std::endl;
                continue;
            }
            bool has_character = false;
            for (auto& pair : debug_teams) {
                if (pair.second->size() > 0) {
                    has_character = true;
                    break;
                }
            }
            if (!has_character) {
                std::cout << "All teams have no characters, add characters first." << std::endl;
                continue;
            }

            FightComponent fight;
            for (auto& pair : debug_teams) {
                fight.add_team(*pair.second);
            }
            std::cout << "\n========== Battle Start ==========" << std::endl;
            fight.start();
            std::cout << "========== Battle End ==========\n" << std::endl;

            debug_teams.clear();
            debug_chars.clear();
            current_team_name.clear();
            std::cout << "All teams and characters cleared." << std::endl;
            continue;
        }

        // Test character attributes
        if (line.substr(0, 5) == "/test") {
            std::string char_name = trim(line.substr(5));
            if (char_name.empty()) {
                std::cout << "Usage: /test <CharName>" << std::endl;
                continue;
            }
            character tmp(char_name);
            tmp.SetRule(SHOW_ATTRIBUTES, true);
            tmp.outputattr();
            std::cout << "Evaluating character " << char_name << " (standard enemy, 30 battles each test)..." << std::endl;
            BenchmarkReport report = evaluate_character_benchmark(char_name, 30);

            // 根据等级设置颜色
            color grade_color;
            if (report.grade == "S") {
                grade_color = color::red;
            } else if (report.grade == "A") {
                grade_color = color::yellow;
            } else if (report.grade == "B") {
                grade_color = color::green;
            } else if (report.grade == "C") {
                grade_color = color::blue;
            } else {
                grade_color = color::cyan;
            }

            std::cout << "\n========== Character Strength Report ==========" << std::endl;
            std::cout << "Character: " << char_name << std::endl;
            std::cout << "Grade: " << textcolor(grade_color) << bold() << report.grade << resetcolor() << std::endl;
            std::cout << "Overall Score: " << std::fixed << std::setprecision(1) << report.final_score << " / 100" << std::endl;
            std::cout << "1v1 Win Rate (30): " << report.win_rate_1v1 << "%" << std::endl;
            std::cout << "1v2 Win Rate (30): " << report.win_rate_1v2 << "%" << std::endl;
            std::cout << "1v3 Win Rate (30): " << report.win_rate_1v3 << "%" << std::endl;
            std::cout << "Static Attribute Score: " << report.static_score << std::endl;
            std::cout << "=============The actions of the character=============" << std::endl;
            const auto& actions = tmp.get_actions();
            if (actions.empty()) {
                std::cout << "No actions available." << std::endl;
            }
            else {
                for (size_t i = 0; i < actions.size(); ++i) {
                    std::cout << i + 1 << ". " << actions[i]->get_name() << std::endl;
                }
            }
            std::cout << "==============================================\n" << std::endl;
            continue;
        }
        // 快速评分（静态属性）
        if (line.substr(0, 6) == "/ftest") {
            std::string args = trim(line.substr(6));
            if (!args.empty()) {
                // 有参数：单次评分
                double score = get_fast_score(args);
                std::cout << "角色 [" << args << "] 快速评分: " << std::fixed << std::setprecision(1) << score << " / 100" << std::endl;
            }
            else {
                // 无参数：进入交互模式
                std::cout << "快速评分交互模式（每行输入一个名字，输入 /end 返回主菜单）" << std::endl;
                std::string name;
                while (true) {
                    std::cout << "fastscore> ";
                    std::getline(std::cin, name);
                    name = trim(name);
                    if (name == "/end") break;
                    if (name.empty()) continue;
                    std::pair<double, std::string>report = evaluate_score(name, 30);
                    std::cout << name << " : " << std::fixed << std::setprecision(1) << report.first << " Grade:" << report.second << std::endl;
                }
            }
            continue;
        }

        // Batch battle simulation
        if (line.substr(0, 9) == "/simulate") {
            std::string args = trim(line.substr(9));
            if (args.empty()) {
                std::cout << "Usage: /simulate HeroName1,HeroName2 vs MonsterName1,MonsterName2 [times]" << std::endl;
                std::cout << "Example: /simulate Arthur,Lancelot vs Goblin,Orc 100" << std::endl;
                continue;
            }

            size_t vs_pos = args.find("vs");
            if (vs_pos == std::string::npos) {
                std::cout << "Error: missing vs separator" << std::endl;
                continue;
            }

            std::string hero_part = trim(args.substr(0, vs_pos));
            std::string rest = trim(args.substr(vs_pos + 2));

            std::string monster_part;
            int battle_count = 30;
            size_t space_pos = rest.find(' ');
            if (space_pos != std::string::npos) {
                monster_part = trim(rest.substr(0, space_pos));
                std::string count_str = trim(rest.substr(space_pos + 1));
                battle_count = std::stoi(count_str);
                if (battle_count <= 0) battle_count = 30;
            }
            else {
                monster_part = rest;
            }

            std::vector<std::string> hero_names = parse_name_list(hero_part);
            std::vector<std::string> monster_names = parse_name_list(monster_part);

            if (hero_names.empty() || monster_names.empty()) {
                std::cout << "Error: hero or monster list is empty" << std::endl;
                continue;
            }

            std::cout << "Starting simulation: " << battle_count << " battles" << std::endl;
            std::cout << "Heroes: " << hero_part << std::endl;
            std::cout << "Monsters: " << monster_part << std::endl;
            std::cout << "Please wait..." << std::endl;

            auto start = std::chrono::steady_clock::now();
            SimulateResult result = run_simulation(hero_names, monster_names, battle_count, false);
            auto end = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(end - start).count();

            std::cout << "\n========== Simulation Results ==========" << std::endl;
            std::cout << "Total: " << battle_count << std::endl;
            std::cout << "Heroes Won: " << result.hero_wins << " (" << std::fixed << std::setprecision(1) << result.hero_win_rate << "%)" << std::endl;
            std::cout << "Monsters Won: " << result.monster_wins << " (" << (100.0 - result.hero_win_rate) << "%)" << std::endl;
            std::cout << "Draws: " << result.draws << std::endl;
            std::cout << "Time: " << elapsed << " seconds" << std::endl;
            std::cout << "========================================\n" << std::endl;
            continue;
        }

        // Batch battle simulation (show battle log)
        if (line.substr(0, 10) == "/simulatex") {
            std::string args = trim(line.substr(10));
            if (args.empty()) {
                std::cout << "用法：/simulatex 英雄名1,英雄名2 vs 怪物名1,怪物名2 [次数]" << std::endl;
                std::cout << "示例：/simulatex 亚瑟,兰斯洛特 vs 哥布林,兽人 100" << std::endl;
                continue;
            }

            size_t vs_pos = args.find("vs");
            if (vs_pos == std::string::npos) {
                std::cout << "错误：缺少 vs 分隔符" << std::endl;
                continue;
            }

            std::string hero_part = trim(args.substr(0, vs_pos));
            std::string rest = trim(args.substr(vs_pos + 2));

            std::string monster_part;
            int battle_count = 30;
            size_t space_pos = rest.find(' ');
            if (space_pos != std::string::npos) {
                monster_part = trim(rest.substr(0, space_pos));
                std::string count_str = trim(rest.substr(space_pos + 1));
                battle_count = std::stoi(count_str);
                if (battle_count <= 0) battle_count = 30;
            }
            else {
                monster_part = rest;
            }

            std::vector<std::string> hero_names = parse_name_list(hero_part);
            std::vector<std::string> monster_names = parse_name_list(monster_part);

            if (hero_names.empty() || monster_names.empty()) {
                std::cout << "错误：英雄或怪物列表为空" << std::endl;
                continue;
            }

            std::cout << "开始模拟战斗（显示日志）：" << battle_count << " 场" << std::endl;
            std::cout << "英雄队：" << hero_part << std::endl;
            std::cout << "怪物队：" << monster_part << std::endl;
            std::cout << "请稍候..." << std::endl;

            auto start = std::chrono::steady_clock::now();
            SimulateResult result = run_simulation(hero_names, monster_names, battle_count, true);
            auto end = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(end - start).count();

            std::cout << "\n========== 模拟结果 ==========" << std::endl;
            std::cout << "总场次: " << battle_count << std::endl;
            std::cout << "英雄胜场: " << result.hero_wins << " (" << std::fixed << std::setprecision(1) << result.hero_win_rate << "%)" << std::endl;
            std::cout << "怪物胜场: " << result.monster_wins << " (" << (100.0 - result.hero_win_rate) << "%)" << std::endl;
            std::cout << "平局: " << result.draws << std::endl;
            std::cout << "耗时: " << elapsed << " 秒" << std::endl;
            std::cout << "================================\n" << std::endl;
            continue;
        }

        if (line.substr(0, 5) == "/find") {
            std::string args = trim(line.substr(5));

            // 无参数：查找已有角色中的加护
            if (args.empty()) {
                bool found = false;
                std::cout << "正在查找已有角色中的加护..." << std::endl;
                for (const auto& ch_ptr : debug_chars) {
                    if (ch_ptr->has_aegis()) {
                        found = true;
                        std::cout << "角色 [" << ch_ptr->get_name() << "] 拥有加护: ";
                        for (const auto& a : ch_ptr->get_aegis()) {
                            std::cout << a << " ";
                        }
                        std::cout << std::endl;
                    }
                }
                if (!found) {
                    std::cout << "未找到任何带有加护的角色。" << std::endl;
                }
                continue;
            }

            // 有参数：尝试解析为数字，生成随机名字并批量测试
            int count = 0;
            try {
                count = std::stoi(args);
                if (count <= 0) {
                    std::cout << "请输入一个正整数。" << std::endl;
                    continue;
                }
            }
            catch (...) {
                std::cout << "用法：/find [数字]   (数字表示随机测试次数，留空则查找已有角色)" << std::endl;
                continue;
            }

            std::cout << "正在生成 " << count << " 个随机名字并检查加护..." << std::endl;
            std::vector<std::string> found_names;
            int total_checked = 0;

            // 进度显示（可选，防止卡顿感）
            customio::progress_bar bar(count, 50, '=', ' ', true);

            for (int i = 0; i < count; ++i) {
                std::string name = random_string(8, 8); // 生成8字符随机名字
                character tmp(name);
                tmp.SetRule(SHOW_ATTRIBUTES, false);
                // 注意：角色构造时会自动调用 setbasicattr() 和 setaegis()，所以可以直接检查
                if (tmp.has_aegis()) {
                    found_names.push_back(name);
                }
                total_checked++;
                bar.update(total_checked);
            }
            bar.finish();

            std::cout << "\n========== 加护检测结果 ==========" << std::endl;
            std::cout << "总测试次数: " << count << std::endl;
            std::cout << "拥有加护的角色数量: " << found_names.size() << std::endl;
            if (!found_names.empty()) {
                std::cout << "名单:" << std::endl;
                for (const auto& name : found_names) {
                    // 可选：再次输出具体加护类型（需要创建临时角色查看）
                    character c(name);
                    c.SetRule(SHOW_ATTRIBUTES, false);
                    std::cout <<  name << ":";
                    for (const auto& a : c.get_aegis()) {
                        std::cout << " " << a;
                    } 
                    std::cout << std::endl;
                }
            }
            std::cout << "==================================\n" << std::endl;
            continue;
        }

        // 导入角色        
        if (line.substr(0, 7) == "/import") {
            std::string args = trim(line.substr(7));
            if (args.empty()) {
                std::cout << "用法：/import <json文件路径>" << std::endl;
                std::cout << "示例：/import characters.json" << std::endl;
                continue;
            }
            bool success = import_characters_from_json(args);
            if (success) {
                std::cout << textcolor(color::green) << "角色导入完成！可使用 [:角色ID] 来调用导入的角色。" << resetcolor() << std::endl;
            }
            continue;
        }
        //寻找高分角色并输出测试结果
        if (line.substr(0, 6) == "/hfind") {
    // 参数解析：/hfind [快速评分门槛] [战斗评分门槛] [生成数量]
    std::string args = trim(line.substr(6));
    int quick_threshold = 70;      // 快速评分门槛
    int battle_threshold = 70;     // 战斗测试门槛
    int total_generate = 10000;    // 生成角色总数，默认10000

    if (!args.empty()) {
        std::stringstream ss(args);
        std::string token;
        std::vector<int> params;
        while (std::getline(ss, token, ' ')) {
            if (token.empty()) continue;
            try {
                params.push_back(std::stoi(token));
            } catch (...) {
                // 忽略非数字参数
            }
        }
        if (params.size() >= 1) quick_threshold = params[0];
        if (params.size() >= 2) battle_threshold = params[1];
        if (params.size() >= 3) total_generate = params[2];
    }

    std::cout << "开始两层筛选：快速评分 >= " << quick_threshold
              << "，战斗测试 >= " << battle_threshold
              << "，生成数量：" << total_generate << std::endl;

    // 第一层：快速评分筛选
    std::vector<std::string> candidates;
    {
        customio::progress_bar bar(total_generate, 50, '=', ' ', true);
        for (int i = 0; i < total_generate; ++i) {
            std::string name = random_string(8, 8);
            double score = get_fast_score(name);
            if (score >= quick_threshold) {
                candidates.push_back(name);
            }
            bar.update(i + 1);
        }
        bar.finish();
    }

    std::cout << "第一层筛选完成，共 " << candidates.size() << " 个角色进入战斗测试。\n";
    if (candidates.empty()) {
        std::cout << "没有角色通过快速评分筛选，请降低门槛。\n";
        continue;
    }

    // 第二层：战斗模拟测试
    struct CharacterRecord {
        std::string name;
        double quick_score;
        BenchmarkReport report;
    };
    std::vector<CharacterRecord> finalists;
    {
        customio::progress_bar battle_bar(candidates.size(), 40, '=', ' ', true);
        for (size_t i = 0; i < candidates.size(); ++i) {
            const auto& name = candidates[i];
            double quick = get_fast_score(name);
            BenchmarkReport rep = evaluate_character_benchmark(name, 10); // 10场快速测试
            if (rep.final_score >= battle_threshold) {
                finalists.push_back({name, quick, rep});
            }
            battle_bar.update(i + 1);
        }
        battle_bar.finish();
    }

    // 按战斗评分降序排序
    std::sort(finalists.begin(), finalists.end(),
        [](const CharacterRecord& a, const CharacterRecord& b) {
            return a.report.final_score > b.report.final_score;
        });

    std::cout << "\n========== 高强度角色检测结果 ==========" << std::endl;
    std::cout << "经过两层筛选，共发现 " << finalists.size() << " 个强力角色：" << std::endl;
    if (finalists.empty()) {
        std::cout << "没有角色达到战斗测试标准。\n";
    } else {
        // 使用 printf 对齐表格
        printf("%-22s %-10s %-10s %-6s %-8s %-8s %-8s\n",
               "名字", "快速评分", "战斗评分", "等级", "1v1胜率", "1v2胜率", "1v3胜率");
        std::cout << std::string(74, '-') << std::endl;

        for (const auto& rec : finalists) {
            const auto& rep = rec.report;
            // 设置等级颜色
            if (rep.grade == "S") std::cout << textcolor(color::red);
            else if (rep.grade == "A") std::cout << textcolor(color::yellow);
            else if (rep.grade == "B") std::cout << textcolor(color::green);
            else std::cout << textcolor(color::cyan);

            printf("%-22s %-10.1f %-10.1f %-6s ",
                   rec.name.c_str(),
                   rec.quick_score,
                   rep.final_score,
                   rep.grade.c_str());

            std::cout << resetcolor();
            printf("%-8.0f%% %-8.0f%% %-8.0f%%\n",
                   rep.win_rate_1v1, rep.win_rate_1v2, rep.win_rate_1v3);
        }
    }
    std::cout << "========================================\n" << std::endl;
    continue;
}
            if (line.rfind("/settheme", 0) == 0) {
            std::string theme_name = trim(line.substr(9));
            if (theme_name.empty()) {
                customio::interactive_theme_selector();
            } else {
                if (customio::set_theme_by_name(theme_name)) {
                    std::cout << "Theme switched to " << theme_name << ".\n";
                } else {
                    std::cout << "Unknown theme. Use /theme to list available themes.\n";
                }
            }
            continue;
        }
        if (line.rfind("/theme", 0) == 0) {
            std::string theme_name = trim(line.substr(6));
            if (theme_name == "") {
                customio::list_available_themes();
            } else {
                if (customio::set_theme_by_name(theme_name)) {
                    std::cout << "Theme switched to " << theme_name << ".\n";
                } else {
                    std::cout << "Unknown theme. Use /theme to list available themes.\n";
                }
            }
            continue;
        }
        if (line.rfind('/cmd', 0) == 0) {
            std::string command = trim(line.substr(6));
            if (command == "") {
                customio::list_available_themes();
            } else {
                system(command.c_str());
            }
            continue;
        }
        std::cout << "未知命令，请重新输入。" << std::endl;
    }
}


//测试新功能……
