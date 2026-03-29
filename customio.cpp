#include "customio.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#endif



namespace customio {

    // ---------- 初始化 ----------
    void init_console() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }

    // ---------- ANSI 辅助 ----------
    static const char* ansi_color_code(color c, bool is_background) {
        switch (c) {
        case color::black:   return is_background ? "\033[40m" : "\033[30m";
        case color::red:     return is_background ? "\033[41m" : "\033[31m";
        case color::green:   return is_background ? "\033[42m" : "\033[32m";
        case color::yellow:  return is_background ? "\033[43m" : "\033[33m";
        case color::blue:    return is_background ? "\033[44m" : "\033[34m";
        case color::magenta: return is_background ? "\033[45m" : "\033[35m";
        case color::cyan:    return is_background ? "\033[46m" : "\033[36m";
        case color::white:   return is_background ? "\033[47m" : "\033[37m";
        case color::reset:   return "\033[0m";
        default:             return "";
        }
    }

    textcolor_manip textcolor(color c) { return { c }; }
    std::ostream& operator<<(std::ostream& os, const textcolor_manip& manip) {
        os << ansi_color_code(manip.col, false);
        return os;
    }

    background_manip background(color c) { return { c }; }
    std::ostream& operator<<(std::ostream& os, const background_manip& manip) {
        os << ansi_color_code(manip.col, true);
        return os;
    }

    resetcolor_manip resetcolor() { return {}; }
    std::ostream& operator<<(std::ostream& os, const resetcolor_manip&) {
        os << ansi_color_code(color::reset, false);
        return os;
    }

    // 文本样式
    bold_manip bold() { return {}; }
    std::ostream& operator<<(std::ostream& os, const bold_manip&) {
        os << "\033[1m";
        return os;
    }

    underline_manip underline() { return {}; }
    std::ostream& operator<<(std::ostream& os, const underline_manip&) {
        os << "\033[4m";
        return os;
    }

    italic_manip italic() { return {}; }
    std::ostream& operator<<(std::ostream& os, const italic_manip&) {
        os << "\033[3m";
        return os;
    }

    // ---------- 光标控制 ----------
    void save_cursor() { std::cout << "\033[s"; }
    void restore_cursor() { std::cout << "\033[u"; }
    void move_to(int row, int col) { std::cout << "\033[" << row << ";" << col << "H"; }
    void move_up(int lines) { std::cout << "\033[" << lines << "A"; }
    void move_down(int lines) { std::cout << "\033[" << lines << "B"; }
    void move_right(int cols) { std::cout << "\033[" << cols << "C"; }
    void move_left(int cols) { std::cout << "\033[" << cols << "D"; }
    void clear_line() { std::cout << "\033[2K\r"; }
    void clear_screen() { std::cout << "\033[2J\033[1;1H"; }

    // ---------- 打字机 ----------
    slow_printer::slow_printer(const std::string& text, unsigned delay_ms) : text_(text), delay_ms_(delay_ms) {}
    std::ostream& operator<<(std::ostream& os, const slow_printer& sp) {
        for (char c : sp.text_) {
            os << c << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(sp.delay_ms_));
        }
        return os;
    }
    slow_printer slow(const std::string& text, unsigned delay_ms) { return { text, delay_ms }; }

    // ---------- 进度条 ----------
    progress_bar::progress_bar(int total, int width, char fill, char empty, bool show_percent)
        : total_(total), width_(width), fill_char_(fill), empty_char_(empty), show_percent_(show_percent) {
    }

    void progress_bar::update(int progress) {
        int pos = progress * width_ / total_;
        std::cout << '[';
        for (int i = 0; i < width_; ++i) {
            if (i < pos) std::cout << fill_char_;
            else if (i == pos && progress < total_) std::cout << '>';
            else std::cout << empty_char_;
        }
        std::cout << ']';
        if (show_percent_) {
            int percent = progress * 100 / total_;
            std::cout << ' ' << percent << '%';
        }
        std::cout << std::flush << '\r';
    }

    void progress_bar::finish() {
        update(total_);
        std::cout << std::endl;
    }

    // ---------- 加载动画（旋转器）----------
    spinner::spinner(const std::string& message) : running_(false), message_(message) {}
    spinner::~spinner() { stop(); }

    void spinner::run() {
        const char frames[] = { '|', '/', '-', '\\' };
        int i = 0;
        while (running_) {
            std::cout << '\r' << message_ << ' ' << frames[i % 4] << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++i;
        }
        std::cout << '\r' << std::string(message_.size() + 2, ' ') << '\r' << std::flush; // 清除
    }

    void spinner::start() {
        if (!running_) {
            running_ = true;
            thread_ = std::thread(&spinner::run, this);
        }
    }

    void spinner::stop() {
        if (running_) {
            running_ = false;
            if (thread_.joinable()) thread_.join();
        }
    }
    //---------- 带提示的输入 ----------
    std::string input(const std::string& prompt = "") {
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }
    // ---------- 密码输入 ----------
    std::string get_password(const std::string& prompt, char mask) {
        std::cout << prompt << std::flush;
        std::string password;

#ifdef _WIN32
        while (true) {
            char ch = _getch();
            if (ch == '\r' || ch == '\n') {
                std::cout << std::endl;
                break;
            }
            else if (ch == '\b' || ch == 127) {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            else {
                password.push_back(ch);
                std::cout << mask << std::flush;
            }
        }
#else
        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        while (true) {
            char ch = getchar();
            if (ch == '\n' || ch == '\r') {
                std::cout << std::endl;
                break;
            }
            else if (ch == 127 || ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            else {
                password.push_back(ch);
                std::cout << mask << std::flush;
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
        return password;
    }

    // ---------- 超时输入 ----------
    std::string read_with_timeout(const std::string& prompt, int timeout_seconds, const std::string& default_value) {
        std::cout << prompt << std::flush;
        std::string input;
        // 简单实现：使用线程等待输入，超时则返回默认值（实际需更复杂处理）
        // 这里为简化，仅提示并返回默认
        std::cout << " (timeout " << timeout_seconds << "s, default: " << default_value << ") ";
        std::getline(std::cin, input);
        if (input.empty()) return default_value;
        return input;
    }

    // ---------- 确认提示 ----------
    bool confirm(const std::string& prompt, bool default_yes) {
        std::string options = default_yes ? " [Y/n]: " : " [y/N]: ";
        std::cout << prompt << options << std::flush;
        std::string answer;
        std::getline(std::cin, answer);
        if (answer.empty()) return default_yes;
        char c = tolower(answer[0]);
        return c == 'y';
    }

    // ---------- 彩色提示 ----------
    std::string prompt(const std::string& prompt_text, color c) {
        std::cout << textcolor(c) << prompt_text << resetcolor() << std::flush;
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    // ---------- 表格输出 ----------
    void print_table(const std::vector<std::vector<std::string>>& data, bool header) {
        if (data.empty()) return;
        size_t cols = data[0].size();
        std::vector<size_t> widths(cols, 0);

        // 计算每列最大宽度
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size() && i < cols; ++i) {
                widths[i] = max(widths[i], row[i].size());
            }
        }

        auto print_separator = [&]() {
            std::cout << '+';
            for (size_t w : widths) {
                std::cout << std::string(w + 2, '-') << '+';
            }
            std::cout << '\n';
            };

        print_separator();
        for (size_t r = 0; r < data.size(); ++r) {
            std::cout << '|';
            for (size_t c = 0; c < cols; ++c) {
                std::cout << ' ' << std::left << std::setw(widths[c]) << data[r][c] << " |";
            }
            std::cout << '\n';
            if (header && r == 0) print_separator();
        }
        print_separator();
    }

    // ---------- 重复输出 ----------
    repeat_manip repeat(int count, const std::string& str) { return { count, str }; }
    std::ostream& operator<<(std::ostream& os, const repeat_manip& rep) {
        for (int i = 0; i < rep.count; ++i) os << rep.str;
        return os;
    }

    // ---------- 日志着色 ----------
    std::ostream& log(log_level level) {
        switch (level) {
        case log_level::INFO:  std::cout << textcolor(color::green) << "[INFO] "; break;
        case log_level::WARN:  std::cout << textcolor(color::yellow) << "[WARN] "; break;
        case log_level::ERR: std::cout << textcolor(color::red) << "[ERROR] "; break;
        }
        return std::cout;
    }
    std::mt19937& get_random_engine() {
        static std::mt19937 engine(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        return engine;
    }

    bool chance(int percent) {
        if (percent <= 0) return false;
        if (percent >= 100) return true;
        auto& rng = get_random_engine();
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= percent;
    }

    bool chance(double probability) {
        if (probability <= 0.0) return false;
        if (probability >= 1.0) return true;
        auto& rng = get_random_engine();
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng) <= probability;
    }

} // namespace customio