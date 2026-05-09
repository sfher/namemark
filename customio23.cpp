#include "customio23.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
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
        cprint("{}{}", ansi_bg(current_theme.background), ansi_fg(adaptive_fg(current_theme.text)));
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
        cprint("{}", prompt);
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    std::string prompt(std::string_view prompt_text, color c) {
        cprint("{}{}{}", ansi_bg(current_theme.background),
            ansi_fg(adaptive_fg(c)), prompt_text);
        cprint("{}", ansi_fg(adaptive_fg(current_theme.text)));
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    bool confirm(std::string_view prompt, bool default_yes) {
        cprint("{} [{}]: ", prompt, default_yes ? "Y/n" : "y/N");
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
        cprint("{}", prompt);
        std::string pwd;
        while (true) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') { cprint("\n"); break; }
            if (ch == '\b' || ch == 127) {
                if (!pwd.empty()) {
                    pwd.pop_back();
                    cprint("\b \b");
                }
            }
            else {
                pwd.push_back((char)ch);
                cprint("{}", mask);
            }
        }
        return pwd;
    }
#else
    std::string get_password(std::string_view prompt, char mask) {
        cprint("{}", prompt);
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
                cprint("\n");
                break;
            }
            else if (ch == 127 || ch == '\b') {
                if (!pwd.empty()) {
                    pwd.pop_back();
                    cprint("\b \b");
                }
            }
            else {
                pwd.push_back(ch);
                cprint("{}", mask);
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
                std::cout << " " << std::left << std::setw(widths[i]) << row[i] << " ";
                if (i < row.size() - 1) std::cout << "|";
            }
            std::cout << "\n";
            if (is_header) {
                for (size_t i = 0; i < row.size(); ++i) {
                    std::cout << std::string(widths[i] + 2, '-');
                    if (i < row.size() - 1) std::cout << "+";
                }
                std::cout << "\n";
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
            cprint("\033[2J\033[1;1H");
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
        cprint("\r[");
        for (int i = 0; i < width_; ++i) {
            if (i < pos) cprint("{}", fill_);
            else if (i == pos && progress < total_) cprint(">");
            else cprint("{}", empty_);
        }
        cprint("]");
        if (show_percent_) {
            int pct = progress * 100 / total_;
            cprint(" {}%", pct);
        }
    }

    void progress_bar::finish() {
        update(total_);
        cprint("\n");
    }

    // ===== operator<< for styled_text =====
    std::ostream& operator<<(std::ostream& os, const styled_text& st) {
        if (has_style(st.style, text_style::bold))      os << "\033[1m";
        if (has_style(st.style, text_style::italic))    os << "\033[3m";
        if (has_style(st.style, text_style::underline)) os << "\033[4m";

        color use_fg = adaptive_fg(st.fg);
        os << ansi_fg(use_fg);
        os << st.text;
        os << "\033[22m\033[23m\033[24m\033[39m";
        os << ansi_fg(adaptive_fg(current_theme.text));
        return os;
    }

    // ===== Spinner =====
    spinner::spinner(std::string_view message) : message_(message) {}
    spinner::~spinner() { stop(); }

    void spinner::start() {
        if (!thread_.joinable()) {
            running_ = true;
            thread_ = std::thread([this]() {
                const char frames[] = { '|', '/', '-', '\\' };
                int i = 0;
                while (running_) {
                    std::cout << "\r" << message_ << " " << frames[i % 4] << std::flush;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ++i;
                }
                std::cout << "\r" << std::string(message_.size() + 4, ' ') << "\r" << std::flush;
            });
        }
    }

    void spinner::stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

} // namespace customio23