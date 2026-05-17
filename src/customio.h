// customio.h — 跨平台终端 I/O 库
// 封装 ANSI 颜色、光标控制、单键输入、菜单选择、进度条等
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

    // ========== 基础工具 ==========
    extern float g_battle_speed;       // 全局战斗速度倍率
    void game_sleep(int milliseconds);  // 按战斗速度倍率调整的延时

    // 随机数（全局 mt19937 引擎）
    std::mt19937& get_random_engine();
    bool chance(int percent);
    bool chance(double probability);

    // ========== 颜色与主题 ==========
    enum class color {
        black, red, green, yellow, blue, magenta, cyan, white, reset, orange
    };

    // 11 色位主题：背景 + 10 种语义色
    struct console_theme {
        color background;
        color text;
        color damage;
        color heal;
        color attack;
        color special;
        color warning;
        color info;
        color error;
        color prompt;
        color title;
    };

    extern const console_theme default_console_theme;
    extern console_theme current_console_theme;

    // 自适应文字色：根据背景亮度自动选择黑/白以保证可读性
    color resolve_text_color(color background, color desired);
    struct textcolor_manip { color col; };
    textcolor_manip adaptive_textcolor(color desired);
    const console_theme& get_console_theme();
    void set_console_theme(const console_theme& theme);
    void apply_console_theme();  // 应用主题背景色并清屏

    // ANSI 流操纵子
    textcolor_manip textcolor(color c);
    std::ostream& operator<<(std::ostream& os, const textcolor_manip& manip);

    struct background_manip { color col; };
    background_manip background(color c);
    std::ostream& operator<<(std::ostream& os, const background_manip& manip);

    // resetcolor 恢复主题背景 + 自适应文字色（非终端默认色）
    struct resetcolor_manip {};
    resetcolor_manip resetcolor();
    std::ostream& operator<<(std::ostream& os, const resetcolor_manip&);

    // 文本样式
    struct bold_manip {};
    bold_manip bold();
    std::ostream& operator<<(std::ostream& os, const bold_manip&);

    struct underline_manip {};
    underline_manip underline();
    std::ostream& operator<<(std::ostream& os, const underline_manip&);

    struct italic_manip {};
    italic_manip italic();
    std::ostream& operator<<(std::ostream& os, const italic_manip&);

    // ========== 光标控制 ==========
    void save_cursor();
    void restore_cursor();
    void move_to(int row, int col);  // 1-based
    void move_up(int lines = 1);
    void move_down(int lines = 1);
    void move_right(int cols = 1);
    void move_left(int cols = 1);
    void clear_line();
    void clear_screen();

    // ========== 菜单选择（核心交互组件） ==========
    // 返回选中索引；ESC 返回 -1；≤9 项时数字键直达，≥10 项时数字缓冲+回车确认
    int menu_select(const std::vector<std::string>& items, const std::string& title = "");

    // ========== 打字机效果 ==========
    class slow_printer {
        std::string text_;
        unsigned delay_ms_;
    public:
        slow_printer(const std::string& text, unsigned delay_ms = 30);
        friend std::ostream& operator<<(std::ostream& os, const slow_printer& sp);
    };
    slow_printer slow(const std::string& text, unsigned delay_ms = 30);

    // ========== 进度条 ==========
    class progress_bar {
        int total_;
        int width_;
        char fill_char_;
        char empty_char_;
        bool show_percent_;
    public:
        progress_bar(int total, int width = 50, char fill = '=', char empty = ' ', bool show_percent = true);
        void update(int progress);
        void finish();
    };

    // ========== 加载动画（独立线程旋转器） ==========
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

    // ========== 输入辅助 ==========
    std::string input(const std::string& prompt);              // 行输入
    std::string get_password(const std::string& prompt = "Password: ", char mask = '*');  // 密码输入
    std::string read_with_timeout(const std::string& prompt, int timeout_seconds, const std::string& default_value = "");
    bool confirm(const std::string& prompt, bool default_yes = true);
    std::string prompt(const std::string& prompt_text, color c = color::reset);  // 彩色提示输入

    // ========== 主题化输出 ==========
    void themed_print(const std::string& text, color fg = color::reset, bool bold = false, bool underline = false, bool italic = false);
    void themed_println(const std::string& text, color fg = color::reset, bool bold = false, bool underline = false, bool italic = false);
    void themed_block(const std::string& block_text, color fg = color::reset, bool bold = false);

    // 表格输出
    void print_table(const std::vector<std::vector<std::string>>& data, bool header = false);

    // 重复字符串
    struct repeat_manip { int count; std::string str; };
    repeat_manip repeat(int count, const std::string& str);
    std::ostream& operator<<(std::ostream& os, const repeat_manip& rep);

    // 日志着色前缀
    enum class log_level { INFO, WARN, ERR };
    std::ostream& log(log_level level);

    // ========== 跨平台单键输入 ==========
    void flush_stdin();  // 清空输入缓冲区
    int  getch();        // 读取单字节（无回显、无缓冲）
    int  read_key();     // 读取按键（处理转义序列：方向键/ESC/回车 → KEY_* 常量）
    void wait_key();     // 等待任意按键

    enum {
        KEY_UP    = 0x100,
        KEY_DOWN  = 0x101,
        KEY_ENTER = 0x10D
    };

    // ========== 初始化 ==========
    void init_console();  // Windows: 启用 UTF-8 + VT 处理

    // ========== 预设主题 ==========
    extern const console_theme light_theme;
    extern const console_theme dark_theme;
    extern const console_theme retro_theme;
    extern const console_theme high_contrast_theme;

    void list_available_themes();
    bool set_theme_by_name(const std::string& theme_name);
    void interactive_theme_selector();

} // namespace customio

// UTF-8 → ANSI 转换（Windows 10+ 设为 UTF-8 CP 后直通）
std::string Utf8ToAnsi(const std::string& utf8_str);

#endif
