#include "customio23.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>

#ifdef _WIN32
#define NOMINMAX          // 阻止 windows.h 定义 min/max 宏
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace customio23 {

    theme current_theme = default_theme;
    float g_battle_speed = 1.2f;

    void set_theme(const theme& t) { current_theme = t; apply_theme(); }
    const theme& get_theme() { return current_theme; }

    void apply_theme() {
        std::print("{}{}", ansi_bg(current_theme.background), ansi_fg(adaptive_fg(current_theme.text)));
        clear_screen();
    }

    static bool is_light_color(color c) {
        return c == color::white || c == color::yellow ||
            c == color::cyan || c == color::magenta || c == color::orange;
    }

    color adaptive_fg(color desired) {
        if (desired == color::reset) return desired;
        color bg = current_theme.background;
        if (bg == desired) {
            return is_light_color(bg) ? color::black : color::white;
        }
        if (is_light_color(bg)) {
            if (desired == color::white || desired == color::yellow)
                return color::black;
        }
        else {
            if (desired == color::black)
                return color::white;
        }
        return desired;
    }

    void game_sleep(int ms) {
        if (ms <= 0 || g_battle_speed <= 0.0f) return;
        auto scaled_ms = static_cast<int>(std::round(ms / g_battle_speed));
        if (scaled_ms <= 0) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(scaled_ms));
    }

    std::mt19937& rng() {
        static std::mt19937 engine(std::random_device{}());
        return engine;
    }

    bool chance(int percent) {
        if (percent <= 0) return false;
        if (percent >= 100) return true;
        std::uniform_int_distribution dist(1, 100);
        return dist(rng()) <= percent;
    }

    bool chance(double p) {
        if (p <= 0.0) return false;
        if (p >= 1.0) return true;
        std::uniform_real_distribution dist(0.0, 1.0);
        return dist(rng()) <= p;
    }

    std::string input(std::string_view prompt) {
        std::print("{}", prompt);
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    std::string prompt(std::string_view prompt_text, color c) {
        std::print("{}{}{}", ansi_bg(current_theme.background),
            ansi_fg(adaptive_fg(c)), prompt_text);
        std::print("{}", ansi_fg(adaptive_fg(current_theme.text)));
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    bool confirm(std::string_view prompt, bool default_yes) {
        std::print("{} [{}]: ", prompt, default_yes ? "Y/n" : "y/N");
        std::string ans;
        std::getline(std::cin, ans);
        auto start = ans.find_first_not_of(" \t");
        if (start == std::string::npos) return default_yes;
        char c = (char)std::tolower((unsigned char)ans[start]);
        return c == 'y';
    }

    // ===== get_password =====
#ifdef _WIN32
    std::string get_password(std::string_view prompt, char mask) {
        std::print("{}", prompt);
        std::string pwd;
        while (true) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') { std::print("\n"); break; }
            if (ch == '\b' || ch == 127) {
                if (!pwd.empty()) {
                    pwd.pop_back();
                    std::print("\b \b");
                }
            }
            else {
                pwd.push_back((char)ch);
                std::print("{}", mask);
            }
        }
        return pwd;
    }
#else
    std::string get_password(std::string_view prompt, char mask) {
        std::print("{}", prompt);
        std::string pwd;

        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        char ch;
        while (true) {
            read(STDIN_FILENO, &ch, 1);
            if (ch == '\n' || ch == '\r') {
                std::print("\n");
                break;
            }
            else if (ch == 127 || ch == '\b') {
                if (!pwd.empty()) {
                    pwd.pop_back();
                    std::print("\b \b");
                }
            }
            else {
                pwd.push_back(ch);
                std::print("{}", mask);
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return pwd;
    }
#endif

    // ===== 日志（改用 cprintln） =====
    void log(log_level lvl, std::string_view msg) {
        color c = color::white;
        std::string_view tag;
        switch (lvl) {
        case log_level::info: c = current_theme.info; tag = "[INFO] "; break;
        case log_level::warn: c = current_theme.warning; tag = "[WARN] "; break;
        case log_level::err:  c = current_theme.error; tag = "[ERROR] "; break;
        }
        cprintln("{} {}", styled(tag, c, text_style::bold), msg);
    }

    // ===== 表格打印 =====
    void print_table(const std::vector<std::vector<std::string>>& data, bool header) {
        if (data.empty()) return;
        size_t cols = data[0].size();
        std::vector<size_t> widths(cols, 0);
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size() && i < cols; ++i) {
                widths[i] = std::max(widths[i], row[i].size());
            }
        }

        auto print_row = [&](const std::vector<std::string>& row, bool is_header) {
            for (size_t i = 0; i < row.size(); ++i) {
                std::print(" {:<{}} ", row[i], widths[i]);
                if (i < row.size() - 1) std::print("|");
            }
            std::print("\n");
            if (is_header) {
                for (size_t i = 0; i < row.size(); ++i) {
                    std::print("{:-<{}}", "", widths[i] + 2);
                    if (i < row.size() - 1) std::print("+");
                }
                std::print("\n");
            }
            };

        if (header && data.size() > 0) {
            print_row(data[0], true);
        }
        size_t start = header ? 1 : 0;
        for (size_t r = start; r < data.size(); ++r) {
            print_row(data[r], false);
        }
    }

    // ===== 菜单（方向键导航，调用 cprint/cprintln） =====
#ifdef _WIN32
    int menu_select(const std::vector<std::string>& items, std::string_view title) {
        if (items.empty()) return -1;
        int selected = 0;
        const int count = (int)items.size();

        auto draw = [&]() {
            clear_screen();
            if (!title.empty()) {
                cprintln("{}", styled(title, current_theme.title, text_style::bold));
            }
            for (int i = 0; i < count; ++i) {
                if (i == selected) {
                    cprintln(" > {}", styled(items[i], current_theme.prompt, text_style::bold));
                }
                else {
                    cprintln("   {}", items[i]);
                }
            }
            };

        draw();
        while (true) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                ch = _getch();
                if (ch == 72) selected = (selected - 1 + count) % count;
                else if (ch == 80) selected = (selected + 1) % count;
            }
            else if (ch == '\r' || ch == '\n') {
                return selected;
            }
            else if (ch == 27) {
                return -1;
            }
            draw();
        }
    }
#else
    int menu_select(const std::vector<std::string>& items, std::string_view title) {
        if (items.empty()) return -1;
        int selected = 0;
        const int count = (int)items.size();

        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        auto draw = [&]() {
            std::print("\033[2J\033[1;1H");
            if (!title.empty()) {
                cprintln("{}", styled(title, current_theme.title, text_style::bold));
            }
            for (int i = 0; i < count; ++i) {
                if (i == selected) {
                    cprintln(" > {}", styled(items[i], current_theme.prompt, text_style::bold));
                }
                else {
                    cprintln("   {}", items[i]);
                }
            }
            };

        draw();
        while (true) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) <= 0) continue;
            if (ch == 27) {
                char seq[2];
                if (read(STDIN_FILENO, seq, 2) != 2) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (seq[0] == '[') {
                    if (seq[1] == 'A') selected = (selected - 1 + count) % count;
                    else if (seq[1] == 'B') selected = (selected + 1) % count;
                }
            }
            else if (ch == '\n' || ch == '\r') {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return selected;
            }
            draw();
        }
    }
#endif

    // ===== 进度条（防御宏：给 std::max 加括号） =====
    progress_bar::progress_bar(int total, int width, char fill, char empty, bool percent)
        : total_((std::max)(1, total)), width_(width), fill_(fill), empty_(empty),
        show_percent_(percent) {
    }

    void progress_bar::update(int progress) {
        progress = std::clamp(progress, 0, total_);
        int pos = progress * width_ / total_;
        std::print("\r[");
        for (int i = 0; i < width_; ++i) {
            if (i < pos) std::print("{}", fill_);
            else if (i == pos && progress < total_) std::print(">");
            else std::print("{}", empty_);
        }
        std::print("]");
        if (show_percent_) {
            int pct = progress * 100 / total_;
            std::print(" {}%", pct);
        }
    }

    void progress_bar::finish() {
        update(total_);
        std::print("\n");
    }

    // ===== Spinner =====
    spinner::spinner(std::string_view message) : message_(message) {}
    spinner::~spinner() { stop(); }

    void spinner::start() {
        if (!thread_.joinable()) {
            thread_ = std::jthread([this](std::stop_token stoken) {
                const char frames[] = { '|', '/', '-', '\\' };
                int i = 0;
                while (!stoken.stop_requested()) {
                    std::print("\r{} {}", message_, frames[i % 4]);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ++i;
                }
                // 清除行（安全填充空格）
                std::print("\r{: <{}}\r", ' ', message_.size() + 4);
                });
        }
    }

    void spinner::stop() {
        thread_.request_stop();
        if (thread_.joinable()) thread_.join();
    }

} // namespace customio23