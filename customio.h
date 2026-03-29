#ifndef CUSTOMIO_H
#define CUSTOMIO_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <chrono>
#include <random>

namespace customio {
    //随机工具
    std::mt19937& get_random_engine();
    bool chance(int percent);           // 整数百分比，如 30 表示 30%
    bool chance(double probability);    // 浮点概率，如 0.3


    // ---------- 颜色 ----------
    enum class color {
        black, red, green, yellow, blue, magenta, cyan, white, reset
    };

    struct textcolor_manip { color col; };
    textcolor_manip textcolor(color c);
    std::ostream& operator<<(std::ostream& os, const textcolor_manip& manip);

    struct background_manip { color col; };
    background_manip background(color c);
    std::ostream& operator<<(std::ostream& os, const background_manip& manip);

    struct resetcolor_manip {};
    resetcolor_manip resetcolor();
    std::ostream& operator<<(std::ostream& os, const resetcolor_manip&);

    // ---------- 文本样式 ----------
    struct bold_manip {};
    bold_manip bold();
    std::ostream& operator<<(std::ostream& os, const bold_manip&);

    struct underline_manip {};
    underline_manip underline();
    std::ostream& operator<<(std::ostream& os, const underline_manip&);

    struct italic_manip {};
    italic_manip italic();
    std::ostream& operator<<(std::ostream& os, const italic_manip&);

    // ---------- 光标控制 ----------
    void save_cursor();
    void restore_cursor();
    void move_to(int row, int col); // 1-based
    void move_up(int lines = 1);
    void move_down(int lines = 1);
    void move_right(int cols = 1);
    void move_left(int cols = 1);
    void clear_line(); // 清除当前行
    void clear_screen();

    // ---------- 打字机效果 ----------
    class slow_printer {
        std::string text_;
        unsigned delay_ms_;
    public:
        slow_printer(const std::string& text, unsigned delay_ms = 30);
        friend std::ostream& operator<<(std::ostream& os, const slow_printer& sp);
    };
    slow_printer slow(const std::string& text, unsigned delay_ms = 30);

    // ---------- 进度条 ----------
    class progress_bar {
        int total_;
        int width_;
        char fill_char_;
        char empty_char_;
        bool show_percent_;
    public:
        progress_bar(int total, int width = 50, char fill = '=', char empty = ' ', bool show_percent = true);
        void update(int progress); // 更新进度并绘制
        void finish(); // 完成，换行
    };

    // ---------- 加载动画（旋转器）----------
    class spinner {
        bool running_;
        std::thread thread_;
        std::string message_;
        void run();
    public:
        spinner(const std::string& message = "");
        ~spinner();
        void start();
        void stop();
    };
    //---------- 带提示的输入 ----------
    std::string input(const std::string& prompt);

    // ---------- 密码输入 ----------
    std::string get_password(const std::string& prompt = "Password: ", char mask = '*');

    // ---------- 带超时的输入 ----------
    std::string read_with_timeout(const std::string& prompt, int timeout_seconds, const std::string& default_value = "");

    // ---------- 确认提示 ----------
    bool confirm(const std::string& prompt, bool default_yes = true);

    // ---------- 彩色提示 ----------
    std::string prompt(const std::string& prompt_text, color c = color::reset);

    // ---------- 表格输出 ----------
    void print_table(const std::vector<std::vector<std::string>>& data, bool header = false);

    // ---------- 重复输出 ----------
    struct repeat_manip {
        int count;
        std::string str;
    };
    repeat_manip repeat(int count, const std::string& str);
    std::ostream& operator<<(std::ostream& os, const repeat_manip& rep);

    // ---------- 日志着色 ----------
    enum class log_level { INFO, WARN, ERR };
    std::ostream& log(log_level level);

    // ---------- 初始化（Windows启用ANSI）----------
    void init_console();

} // namespace customio

#endif