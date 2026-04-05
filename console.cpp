#include "entity.h"
#include "customio.h"
#include "act.h"
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
#include <cctype>

using namespace customio;

// Forward declaration of helper functions
std::string trim(const std::string& s);

// ========== 随机字符串生成 ==========
static std::string random_string(int min_len, int max_len) {
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
    double win_rate_1v4;   // 对4个标准敌人胜率
    double final_score;
    std::string grade;
    int static_score;
    void output() {
        std::cout << final_score;
    }
};

BenchmarkReport evaluate_character_benchmark(const std::string& char_name, int battles_per_test = 30) {
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
        auto res = run_1vN_battle(char_name, 4);
        if (res.win) wins++;
    }
    report.win_rate_1v4 = (double)wins / battles_per_test * 100.0;

    report.final_score = report.win_rate_1v1 * 0.4 + report.win_rate_1v2 * 0.4 + report.win_rate_1v4 * 0.2;

    if (report.final_score >= 80) report.grade = "S";
    else if (report.final_score >= 60) report.grade = "A";
    else if (report.final_score >= 45) report.grade = "B";
    else report.grade = "C";

    character tmp(char_name);
    tmp.SetRule(SHOW_ATTRIBUTES, false);
    report.static_score = calc_static_score(tmp);

    return report;
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
        Team heroTeam("英雄队");
        std::vector<std::unique_ptr<character>> hero_chars;
        for (const auto& name : hero_names) {
            auto ch = std::make_unique<character>(name);
            if (!show_fightlog) ch->SetRule(BATTLE_WITHOUT_OUTPUT, true);
            if (!show_fightlog) ch->SetRule(SHOW_ATTRIBUTES, false);
            heroTeam.add_character(*ch);
            hero_chars.push_back(std::move(ch));
        }

        Team monsterTeam("怪物队");
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

// ========== 调试控制台 ==========
void debug_console() {
    std::cout << bold() << textcolor(color::green)
        << "\n===== 调试控制台已启动 =====" << resetcolor() << std::endl;
    std::cout << "命令列表：" << std::endl;
    std::cout << "  @<队名>                - 创建队伍（如 @英雄队）" << std::endl;
    std::cout << "  <角色名> @<队名>       - 创建角色并加入指定队伍" << std::endl;
    std::cout << "  /fight                 - 开始战斗（所有队伍对战）" << std::endl;
    std::cout << "  /test <角色名>         - 查看角色属性（基于名字哈希）" << std::endl;
    std::cout << "  /score <角色名>        - 评估角色强度（标准敌人数值评分，耗时较长）" << std::endl;
    std::cout << "  /fastscore [角色名]    - 快速评分（静态属性）。无参数则进入交互模式，输入名字即可评分，/end返回" << std::endl;
    std::cout << "  /simulate <英雄列表> vs <怪物列表> [次数] - 批量模拟战斗" << std::endl;
    std::cout << "  /find寻找带有加护的名字" << std::endl;
    std::cout << "  /end                   - 退出调试控制台" << std::endl;
    std::cout << "提示：角色名不能包含空格，队伍名需以@开头" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::string line;
    std::string current_team_name;
    while (true) {
        std::cout << textcolor(color::cyan) << "debug> " << resetcolor();
        std::getline(std::cin, line);
        line = trim(line);
        if (line.empty()) continue;

        if (line == "/end") {
            std::cout << "退出调试控制台。" << std::endl;
            break;
        }

        // 创建队伍
        if (line[0] == '@') {
            std::string team_name = line.substr(1);
            if (debug_teams.find(team_name) != debug_teams.end()) {
                std::cout << "队伍 " << team_name << " 已存在，无法重复创建。" << std::endl;
            }
            else {
                auto team = std::make_unique<Team>(team_name);
                team->set_name(team_name);
                debug_teams[team_name] = std::move(team);
                std::cout << "队伍 [" << team_name << "] 创建成功。" << std::endl;
            }
            current_team_name = team_name;
            continue;
        }

        // 创建角色并加入队伍（显式指定队伍）
        size_t at_pos = line.find('@');
        if (at_pos != std::string::npos) {
            std::string char_name = trim(line.substr(0, at_pos));
            std::string team_name = trim(line.substr(at_pos + 1));
            if (char_name.empty() || team_name.empty()) {
                std::cout << "格式错误：请使用 \"角色名 @队名\"。" << std::endl;
                continue;
            }
            auto it = debug_teams.find(team_name);
            if (it == debug_teams.end()) {
                std::cout << "队伍 " << team_name << " 不存在，请先创建队伍（@" << team_name << "）。" << std::endl;
                continue;
            }
            auto ch = std::make_unique<character>(char_name);
            ch->SetRule(SHOW_ATTRIBUTES, false);
            it->second->add_character(*ch);
            debug_chars.push_back(std::move(ch));
            std::cout << "角色 [" << char_name << "] 已加入队伍 [" << team_name << "]。" << std::endl;
            continue;
        }

        // 如果以普通字符开头且不是命令，则加入当前活跃队伍
        if (!line.empty() && line[0] != '/') {
            std::string char_name = line;
            if (current_team_name.empty()) {
                std::cout << "没有活跃队伍，无法将角色加入。请先创建队伍（例如：@队伍名）或使用 \"角色名 @队伍名\"。" << std::endl;
                continue;
            }
            auto it = debug_teams.find(current_team_name);
            if (it == debug_teams.end()) {
                std::cout << "当前活跃队伍不存在，请重新创建队伍（@" << current_team_name << "）。" << std::endl;
                current_team_name.clear();
                continue;
            }
            auto ch = std::make_unique<character>(char_name);
            ch->SetRule(SHOW_ATTRIBUTES, false);
            it->second->add_character(*ch);
            debug_chars.push_back(std::move(ch));
            std::cout << "角色 [" << char_name << "] 已加入队伍 [" << current_team_name << "]。" << std::endl;
            continue;
        }

        // 战斗（使用当前已创建的所有队伍）
        if (line == "/fight") {
            if (debug_teams.empty()) {
                std::cout << "没有队伍，无法战斗。请先创建队伍并加入角色。" << std::endl;
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
                std::cout << "所有队伍都没有角色，请先添加角色。" << std::endl;
                continue;
            }

            FightComponent fight;
            for (auto& pair : debug_teams) {
                fight.add_team(*pair.second);
            }
            std::cout << "\n========== 战斗开始 ==========" << std::endl;
            fight.start();
            std::cout << "========== 战斗结束 ==========\n" << std::endl;

            debug_teams.clear();
            debug_chars.clear();
            current_team_name.clear();
            std::cout << "所有队伍和角色已清空。" << std::endl;
            continue;
        }

        // 测试角色属性
        if (line.substr(0, 5) == "/test") {
            std::string char_name = trim(line.substr(5));
            if (char_name.empty()) {
                std::cout << "用法：/test <角色名>" << std::endl;
                continue;
            }
            character tmp(char_name);
            tmp.SetRule(SHOW_ATTRIBUTES, true);
            tmp.outputattr();
            continue;
        }

        // 角色评分（精确，战斗）
        if (line.substr(0, 6) == "/score") {
            std::string char_name = trim(line.substr(6));
            if (char_name.empty()) {
                std::cout << "用法：/score <角色名>" << std::endl;
                continue;
            }
            std::cout << "正在评估角色 " << char_name << "（标准敌人，每项30场）..." << std::endl;
            BenchmarkReport report = evaluate_character_benchmark(char_name, 30);

            std::cout << "\n========== 角色强度报告 ==========" << std::endl;
            std::cout << "角色名称: " << char_name << std::endl;
            std::cout << "最终评级: " << report.grade << std::endl;
            std::cout << "综合评分: " << std::fixed << std::setprecision(1) << report.final_score << " / 100" << std::endl;
            std::cout << "1v1 胜率 (30场): " << report.win_rate_1v1 << "%" << std::endl;
            std::cout << "1v2 胜率 (30场): " << report.win_rate_1v2 << "%" << std::endl;
            std::cout << "1v4 胜率 (30场): " << report.win_rate_1v4 << "%" << std::endl;
            std::cout << "静态属性评分: " << report.static_score << std::endl;
            std::cout << "===================================\n" << std::endl;
            continue;
        }

        // 快速评分（静态属性）
        if (line.substr(0, 10) == "/fastscore") {
            std::string args = trim(line.substr(10));
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
                    double score = get_fast_score(name);
                    std::cout << name << " : " << std::fixed << std::setprecision(1) << score << std::endl;
                }
            }
            continue;
        }

        // 批量模拟战斗（静默）
        if (line.substr(0, 9) == "/simulate") {
            std::string args = trim(line.substr(9));
            if (args.empty()) {
                std::cout << "用法：/simulate 英雄名1,英雄名2 vs 怪物名1,怪物名2 [次数]" << std::endl;
                std::cout << "示例：/simulate 亚瑟,兰斯洛特 vs 哥布林,兽人 100" << std::endl;
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

            std::cout << "开始模拟战斗：" << battle_count << " 场" << std::endl;
            std::cout << "英雄队：" << hero_part << std::endl;
            std::cout << "怪物队：" << monster_part << std::endl;
            std::cout << "请稍候..." << std::endl;

            auto start = std::chrono::steady_clock::now();
            SimulateResult result = run_simulation(hero_names, monster_names, battle_count, false);
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

        // 批量模拟战斗（显示战斗日志）
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
                std::string name = random_string(3, 12);
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
                std::cout << "名单：" << std::endl;
                for (const auto& name : found_names) {
                    // 可选：再次输出具体加护类型（需要创建临时角色查看）
                    character c(name);
                    c.SetRule(SHOW_ATTRIBUTES, false);
                    std::cout << "  " << name << " : ";
                    for (const auto& a : c.get_aegis()) {
                        std::cout << a << " ";
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << "==================================\n" << std::endl;
            continue;
        }

        std::cout << "未知命令，请重新输入。" << std::endl;
    }
}