// cli.cpp — Namemark command-line mode
#include "cli.h"
#include "console.h"
#include "entity.h"
#include "act.h"
#include "customio.h"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace customio;

// ========== Help ==========
void print_cli_help() {
    std::cout << "Namemark CLI\n\n";
    std::cout << "Usage: namemark <command> [args...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  test <name>              Character evaluation (benchmark)\n";
    std::cout << "  ftest <name>             Quick static score\n";
    std::cout << "  find [count]             Find characters with aegis\n";
    std::cout << "  hfind [q] [b] [n]       High-score character search\n";
    std::cout << "  simulate <a,b> vs <c,d> [times]  Batch simulation\n";
    std::cout << "  simulatex <a,b> vs <c,d> [times] Simulation with battle log\n";
    std::cout << "  import <file>            Import characters from JSON\n";
    std::cout << "  console                  Enter interactive debug console\n";
    std::cout << "  help                     Show this help\n\n";
    std::cout << "No arguments launches the interactive game.\n";
}

// ========== Command implementations ==========

static int cmd_test(const char* name) {
    if (!name || !*name) {
        std::cerr << "Usage: namemark test <CharacterName>\n";
        return 1;
    }
    character tmp(name);
    tmp.SetRule(SHOW_ATTRIBUTES, true);
    tmp.outputattr();

    std::cout << "Evaluating " << name << " (30 battles per test)...\n";
    BenchmarkReport report = evaluate_character_benchmark(name, 30);

    color grade_color;
    if (report.grade == "S") grade_color = color::red;
    else if (report.grade == "A") grade_color = color::yellow;
    else if (report.grade == "B") grade_color = color::green;
    else if (report.grade == "C") grade_color = color::blue;
    else grade_color = color::cyan;

    std::cout << "\n========== Character Strength Report ==========\n";
    std::cout << "Character: " << name << "\n";
    std::cout << "Grade: " << textcolor(grade_color) << bold() << report.grade << resetcolor() << "\n";
    std::cout << "Overall Score: " << std::fixed << std::setprecision(1) << report.final_score << " / 100\n";
    std::cout << "1v1 Win Rate: " << report.win_rate_1v1 << "%\n";
    std::cout << "1v2 Win Rate: " << report.win_rate_1v2 << "%\n";
    std::cout << "1v3 Win Rate: " << report.win_rate_1v3 << "%\n";
    std::cout << "Static Score: " << report.static_score << "\n";

    std::cout << "===== Actions =====\n";
    const auto& actions = tmp.get_actions();
    for (size_t i = 0; i < actions.size(); ++i)
        std::cout << i + 1 << ". " << actions[i]->get_name() << "\n";
    std::cout << "========================================\n";
    return 0;
}

static int cmd_ftest(const char* name) {
    if (!name || !*name) {
        std::cerr << "Usage: namemark ftest <CharacterName>\n";
        return 1;
    }
    auto [score, grade] = evaluate_score(name, 30);
    std::cout << std::fixed << std::setprecision(1);
    std::cout << name << " : " << score << "  Grade: " << grade << "\n";
    return 0;
}

static int cmd_find(int count) {
    if (count <= 0) {
        std::cerr << "Usage: namemark find <count>\n";
        return 1;
    }
    std::cout << "Generating " << count << " random names and checking aegis...\n";

    std::vector<std::string> found_names;
    progress_bar bar(count, 50, '=', ' ', true);

    for (int i = 0; i < count; ++i) {
        std::string name = random_string(8, 8);
        character tmp(name);
        tmp.SetRule(SHOW_ATTRIBUTES, false);
        if (tmp.has_aegis())
            found_names.push_back(name);
        bar.update(i + 1);
    }
    bar.finish();

    std::cout << "\n========== Aegis Detection Results ==========\n";
    std::cout << "Total tested: " << count << "\n";
    std::cout << "Characters with aegis: " << found_names.size() << "\n";
    for (const auto& name : found_names) {
        character c(name);
        c.SetRule(SHOW_ATTRIBUTES, false);
        std::cout << "  " << name << ":";
        for (const auto& a : c.get_aegis())
            std::cout << " " << a;
        std::cout << "\n";
    }
    std::cout << "========================================\n";
    return 0;
}

static int cmd_hfind(int quick_threshold, int battle_threshold, int total_generate) {
    std::cout << "Two-layer search: quick >= " << quick_threshold
              << ", battle >= " << battle_threshold
              << ", generate: " << total_generate << "\n";

    std::vector<std::string> candidates;
    {
        progress_bar bar(total_generate, 50, '=', ' ', true);
        for (int i = 0; i < total_generate; ++i) {
            std::string name = random_string(8, 8);
            if (get_fast_score(name) >= quick_threshold)
                candidates.push_back(name);
            bar.update(i + 1);
        }
        bar.finish();
    }

    std::cout << "Layer 1 passed: " << candidates.size() << " characters\n";
    if (candidates.empty()) {
        std::cout << "No characters passed quick score filter.\n";
        return 0;
    }

    struct Record { std::string name; double quick_score; BenchmarkReport report; };
    std::vector<Record> finalists;

    {
        progress_bar battle_bar(candidates.size(), 40, '=', ' ', true);
        for (size_t i = 0; i < candidates.size(); ++i) {
            auto& name = candidates[i];
            double quick = get_fast_score(name);
            BenchmarkReport rep = evaluate_character_benchmark(name, 10);
            if (rep.final_score >= battle_threshold)
                finalists.push_back({name, quick, rep});
            battle_bar.update(i + 1);
        }
        battle_bar.finish();
    }

    std::sort(finalists.begin(), finalists.end(),
        [](const Record& a, const Record& b) { return a.report.final_score > b.report.final_score; });

    std::cout << "\n========== High-Power Character Results ==========\n";
    std::cout << "Found " << finalists.size() << " powerful characters:\n";
    if (finalists.empty()) {
        std::cout << "No characters passed battle test.\n";
    } else {
        std::cout << std::left
                  << std::setw(22) << "Name"
                  << std::setw(10) << "Quick"
                  << std::setw(10) << "Battle"
                  << std::setw(6)  << "Grade"
                  << std::setw(8)  << "1v1%"
                  << std::setw(8)  << "1v2%"
                  << std::setw(8)  << "1v3%\n";
        std::cout << std::string(74, '-') << "\n";

        for (const auto& rec : finalists) {
            const auto& rep = rec.report;
            if (rep.grade == "S") std::cout << textcolor(color::red);
            else if (rep.grade == "A") std::cout << textcolor(color::yellow);
            else if (rep.grade == "B") std::cout << textcolor(color::green);
            else std::cout << textcolor(color::cyan);

            std::cout << std::left
                      << std::setw(22) << rec.name
                      << std::fixed << std::setprecision(1)
                      << std::setw(10) << rec.quick_score
                      << std::setw(10) << rep.final_score
                      << std::setw(6)  << rep.grade;
            std::cout << resetcolor();
            std::cout << std::setw(8) << (std::to_string((int)rep.win_rate_1v1) + "%")
                      << std::setw(8) << (std::to_string((int)rep.win_rate_1v2) + "%")
                      << std::setw(8) << (std::to_string((int)rep.win_rate_1v3) + "%") << "\n";
        }
    }
    std::cout << "========================================\n";
    return 0;
}

static int cmd_simulate(int argc, char* argv[], bool show_log) {
    // Parse: namemark simulate hero1,hero2 vs monster1,monster2 [times]
    // Remaining args after "simulate": hero1,hero2 vs monster1,monster2 [times]
    if (argc < 5) { // need at least: simulate hero1 vs monster1
        std::cerr << "Usage: namemark simulate <HeroName1,HeroName2,...> vs <MonsterName1,...> [times]\n";
        return 1;
    }

    // Reconstruct args into a single string (args[2..argc-1])
    std::string args = argv[2];
    for (int i = 3; i < argc; ++i)
        args += " " + std::string(argv[i]);

    size_t vs_pos = args.find("vs");
    if (vs_pos == std::string::npos) {
        std::cerr << "Error: missing 'vs' separator\n";
        return 1;
    }

    std::string hero_part = trim(args.substr(0, vs_pos));
    std::string rest = trim(args.substr(vs_pos + 2));

    std::string monster_part;
    int battle_count = 30;
    size_t space_pos = rest.find(' ');
    if (space_pos != std::string::npos) {
        monster_part = trim(rest.substr(0, space_pos));
        std::string count_str = trim(rest.substr(space_pos + 1));
        try { battle_count = std::stoi(count_str); if (battle_count <= 0) battle_count = 30; }
        catch (...) {}
    } else {
        monster_part = rest;
    }

    std::vector<std::string> hero_names = parse_name_list(hero_part);
    std::vector<std::string> monster_names = parse_name_list(monster_part);

    if (hero_names.empty() || monster_names.empty()) {
        std::cerr << "Error: hero or monster list is empty\n";
        return 1;
    }

    std::cout << "Starting simulation: " << battle_count << " battles\n";
    std::cout << "Heroes: " << hero_part << "\n";
    std::cout << "Monsters: " << monster_part << "\n\n";

    auto start = std::chrono::steady_clock::now();
    SimulateResult result = run_simulation(hero_names, monster_names, battle_count, show_log);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n========== Results ==========\n";
    std::cout << "Total: " << battle_count << "\n";
    std::cout << "Heroes Won: " << result.hero_wins
              << " (" << std::fixed << std::setprecision(1) << result.hero_win_rate << "%)\n";
    std::cout << "Monsters Won: " << result.monster_wins
              << " (" << (100.0 - result.hero_win_rate) << "%)\n";
    std::cout << "Draws: " << result.draws << "\n";
    std::cout << "Time: " << elapsed << " seconds\n";
    std::cout << "============================\n";
    return 0;
}

static int cmd_import(const char* path) {
    if (!path || !*path) {
        std::cerr << "Usage: namemark import <json_file_path>\n";
        return 1;
    }
    bool ok = import_characters_from_json(path);
    if (ok)
        std::cout << textcolor(color::green) << "Import successful!" << resetcolor() << "\n";
    else
        std::cerr << "Import failed.\n";
    return ok ? 0 : 1;
}

// ========== Main dispatcher ==========

int cli_mode(int argc, char* argv[]) {
    if (argc < 2) {
        print_cli_help();
        return 0;
    }

    std::string cmd = argv[1];

    // help aliases
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_cli_help();
        return 0;
    }

    // test
    if (cmd == "test") {
        return cmd_test(argc > 2 ? argv[2] : nullptr);
    }

    // ftest
    if (cmd == "ftest") {
        return cmd_ftest(argc > 2 ? argv[2] : nullptr);
    }

    // find
    if (cmd == "find") {
        int count = 1000;
        if (argc > 2) {
            try { count = std::stoi(argv[2]); if (count <= 0) count = 1000; }
            catch (...) {}
        }
        return cmd_find(count);
    }

    // hfind [quick_threshold] [battle_threshold] [total_generate]
    if (cmd == "hfind") {
        int q = 70, b = 70, n = 10000;
        if (argc > 2) { try { q = std::stoi(argv[2]); } catch (...) {} }
        if (argc > 3) { try { b = std::stoi(argv[3]); } catch (...) {} }
        if (argc > 4) { try { n = std::stoi(argv[4]); } catch (...) {} }
        return cmd_hfind(q, b, n);
    }

    // simulate
    if (cmd == "simulate") {
        return cmd_simulate(argc, argv, false);
    }

    // simulatex
    if (cmd == "simulatex") {
        return cmd_simulate(argc, argv, true);
    }

    // import
    if (cmd == "import") {
        return cmd_import(argc > 2 ? argv[2] : nullptr);
    }

    // console — enter interactive debug console
    if (cmd == "console") {
        init_console();
        debug_console();
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\nUse 'namemark help' for usage.\n";
    return 1;
}
