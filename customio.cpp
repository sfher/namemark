#include "customio.h"
#include "console.h"
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

    float g_battle_speed = 1.0f; // 默认正常速度

     void game_sleep(int milliseconds) {
        if (milliseconds <= 0) return;
        auto adjusted = std::chrono::milliseconds(static_cast<int>(milliseconds * g_battle_speed));
        std::this_thread::sleep_for(adjusted);
    }

    const console_theme default_console_theme = {
        color::black,   // background
        color::black,   // text
        color::red,     // damage
        color::green,   // heal
        color::cyan,    // attack
        color::magenta, // special
        color::yellow,  // warning
        color::blue,    // info
        color::red,     // error
        color::cyan,    // prompt
        color::yellow   // title
    };

    // 由于color没有orange，修正为color::yellow
    // 修正版：
    const console_theme light_theme = {
        color::white,
        color::black,
        color::red,
        color::green,
        color::blue,
        color::magenta,
        color::yellow,
        color::blue,
        color::red,
        color::blue,
        color::black
    };

    // 深色背景主题（黑底绿字，经典）
    const console_theme dark_theme = {
        color::black,   // background
        color::green,   // text
        color::red,     // damage
        color::green,   // heal
        color::cyan,    // attack
        color::magenta, // special
        color::yellow,  // warning
        color::cyan,    // info
        color::red,     // error
        color::cyan,    // prompt
        color::yellow   // title
    };

    // 复古主题（琥珀色风格）
    const console_theme retro_theme = {
        color::black,   // background
        color::yellow,  // text (琥珀色)
        color::red,     // damage
        color::green,   // heal
        color::yellow,  // attack
        color::magenta, // special
        color::red,     // warning
        color::yellow,  // info
        color::red,     // error
        color::yellow,  // prompt
        color::yellow   // title
    };

    // 高对比度主题（黄底黑字，无障碍）
    const console_theme high_contrast_theme = {
        color::yellow,  // background
        color::black,   // text
        color::red,     // damage
        color::green,   // heal
        color::blue,    // attack
        color::magenta, // special
        color::red,     // warning
        color::blue,    // info
        color::red,     // error
        color::blue,    // prompt
        color::black    // title
    };

	console_theme current_console_theme = default_console_theme;
    // 主题选择接口
    void list_available_themes();                  // 列出所有可用主题
    bool set_theme_by_name(const std::string& theme_name); // 通过名称设置主题，返回是否成功
    void interactive_theme_selector();             // 交互式主题选择菜单

    void list_available_themes() {
        std::cout << "Available themes:\n"
            << "  default  - Default dark theme (black bg, white text)\n"
            << "  light    - Light background (white bg, black text)\n"
            << "  dark     - Dark green theme (black bg, green text)\n"
            << "  retro    - Retro amber style (black bg, yellow text)\n"
            << "  highcontrast - High contrast (yellow bg, black text)\n";
    }

    bool set_theme_by_name(const std::string& theme_name) {
        std::string name = theme_name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == "default") {
            current_console_theme = default_console_theme;
        }
        else if (name == "light") {
            current_console_theme = light_theme;
        }
        else if (name == "dark") {
            current_console_theme = dark_theme;
        }
        else if (name == "retro") {
            current_console_theme = retro_theme;
        }
        else if (name == "highcontrast") {
            current_console_theme = high_contrast_theme;
        }
        else {
            return false;
        }
        apply_console_theme();
        return true;
    }

    void apply_console_theme() {
        const auto& theme = current_console_theme;
        std::cout << background(theme.background) << adaptive_textcolor(theme.text);
        clear_screen();
    }

    void themed_print(const std::string& text, color fg, bool bold_flag, bool underline_flag, bool italic_flag) {
        const auto& theme = current_console_theme;
        std::cout << background(theme.background);
        if (bold_flag) std::cout << bold();
        if (underline_flag) std::cout << underline();
        if (italic_flag) std::cout << italic();
        std::cout << adaptive_textcolor(fg) << text;
        // 重置文本样式，恢复默认文本颜色，保持主题背景色不变
        std::cout << "\033[22m\033[24m\033[23m" << adaptive_textcolor(theme.text);
    }

    void themed_println(const std::string& text, color fg, bool bold_flag, bool underline_flag, bool italic_flag) {
        themed_print(text, fg, bold_flag, underline_flag, italic_flag);
        std::cout << std::endl;
    }

    void themed_block(const std::string& block_text, color fg, bool bold_flag) {
        std::stringstream ss(block_text);
        std::string line;
        while (std::getline(ss, line)) {
            themed_println(line, fg, bold_flag, false, false);
        }
    }

    void interactive_theme_selector() {
        std::cout << "===== Console Theme Selector =====\n";
        list_available_themes();
        std::cout << "Enter theme name (or 'cancel'): ";
        std::string input;
        std::getline(std::cin, input);
        if (input == "cancel") {
            std::cout << "Theme unchanged.\n";
            return;
        }
        if (set_theme_by_name(input)) {
            std::cout << "Theme changed to '" << input << "'.\n";
        }
        else {
            std::cout << "Unknown theme. Using current theme.\n";
        }
    }

    static bool is_light_color(color c) {
        return c == color::white || c == color::yellow || c == color::cyan || c == color::magenta || c == color::orange;
    }

    color resolve_text_color(color background, color desired) {
        if (desired == color::reset) return desired;
        if (background == desired) {
            return is_light_color(background) ? color::black : color::white;
        }
        if (is_light_color(background)) {
            if (desired == color::white || desired == color::yellow) {
                return color::black;
            }
        }
        else {
            if (desired == color::black) {
                return color::white;
            }
        }
        return desired;
    }

    textcolor_manip adaptive_textcolor(color desired) {
        return textcolor(resolve_text_color(current_console_theme.background, desired));
    }

    const console_theme& get_console_theme() {
        return current_console_theme;
    }

    void set_console_theme(const console_theme& theme) {
        current_console_theme = theme;
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
        case color::orange:  return is_background ? "\033[43m" : "\033[33m"; // 映射到黄色
        case color::reset:   return is_background ? "\033[49m" : "\033[39m";
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
        const auto& theme = current_console_theme;
        // 重置文本样式
        os << "\033[22m\033[24m\033[23m";
        // 恢复为当前主题的默认颜色，而非系统终端默认
        os << background(theme.background) << adaptive_textcolor(theme.text);
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
        const auto& theme = get_console_theme();
        std::cout << background(theme.background)
            << adaptive_textcolor(c)
            << prompt_text
            << adaptive_textcolor(theme.text) // 恢复默认文本颜色，保持主题背景色不变
            << std::flush;
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    // ---------- 表格输出 ----------
    void print_table(const std::vector<std::vector<std::string>>& data, bool header) {
        if (data.empty()) return;
        size_t cols = data[0].size();
        std::vector<size_t> widths(cols, 0);


        // for (const auto& row : data) {
        //     for (size_t i = 0; i < row.size() && i < cols; ++i) {
        //         widths[i] = std::max(widths[i], row[i].size());
        //     }
        // }

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
        const auto& theme = get_console_theme();
        switch (level) {
        case log_level::INFO:  std::cout << adaptive_textcolor(theme.info) << "[INFO] "; break;
        case log_level::WARN:  std::cout << adaptive_textcolor(theme.warning) << "[WARN] "; break;
        case log_level::ERR:   std::cout << adaptive_textcolor(theme.error) << "[ERROR] "; break;
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

  int menu_select(const std::vector<std::string>& items, const std::string& title) {
    if (items.empty()) return -1;

    const auto& theme = get_console_theme();
    int selected = 0;
    bool running = true;

    auto redraw = [&]() {
        clear_screen();
        if (!title.empty()) {
            std::cout << adaptive_textcolor(theme.title) << bold() << title << resetcolor() << std::endl;
        }
        for (size_t i = 0; i < items.size(); ++i) {
            if (i == selected) {
                std::cout << background(theme.prompt) << adaptive_textcolor(theme.background);
            }
            std::cout << "  " << (i == selected ? ">" : " ") << " " << items[i] << "  ";
            if (i == selected) {
                std::cout << resetcolor();
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        std::cout << "↑↓: 移动  Enter: 确认" << std::endl;
    };

    redraw();
    while (running) {
        int ch = _getch();
        if (ch == 224) {
            ch = _getch();
            if (ch == 72 && selected > 0) selected--;
            else if (ch == 80 && selected < (int)items.size() - 1) selected++;
            redraw();
        } else if (ch == 13) {
            running = false;
        }
    }
    return selected;
}
    
} // namespace customio

std::string Utf8ToAnsi(const std::string& utf8_str) {
    if (utf8_str.empty()) return "";

    // UTF-8 转 UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wlen == 0) return utf8_str;
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], wlen);

    // UTF-16 转 ANSI (CP_ACP)
    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (alen == 0) return utf8_str;
    std::string result(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], alen, nullptr, nullptr);

    // 去除末尾 '\0'
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}
