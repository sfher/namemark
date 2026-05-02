#ifndef CUSTOMIO23_H
#define CUSTOMIO23_H

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <print>

namespace customio23 {

    // ---- 颜色枚举 ----
    enum class color : uint8_t {
        black, red, green, yellow, blue, magenta, cyan, white, reset, orange
    };

    // ---- 主题定义 ----
    struct theme {
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

    inline constexpr theme default_theme{
        .background = color::black,
        .text = color::white,
        .damage = color::red,
        .heal = color::green,
        .attack = color::cyan,
        .special = color::magenta,
        .warning = color::yellow,
        .info = color::blue,
        .error = color::red,
        .prompt = color::cyan,
        .title = color::yellow
    };

    inline constexpr theme light_theme{
        .background = color::white,
        .text = color::black,
        .damage = color::red,
        .heal = color::green,
        .attack = color::blue,
        .special = color::magenta,
        .warning = color::yellow,
        .info = color::blue,
        .error = color::red,
        .prompt = color::blue,
        .title = color::black
    };

    // ---- 全局主题管理 ----
    extern theme current_theme;
    extern float g_battle_speed;

    void set_theme(const theme& t);
    const theme& get_theme();
    void apply_theme();

    color adaptive_fg(color desired);

    // ---- 样式描述 ----
    enum class text_style : uint8_t {
        normal = 0,
        bold = 1 << 0,
        italic = 1 << 1,
        underline = 1 << 2
    };

    constexpr text_style operator|(text_style a, text_style b) {
        return static_cast<text_style>(std::to_underlying(a) | std::to_underlying(b));
    }
    constexpr bool has_style(text_style s, text_style flag) {
        return (std::to_underlying(s) & std::to_underlying(flag)) != 0;
    }

    struct styled_text {
        std::string_view text;
        color fg;
        text_style style;
    };

    constexpr styled_text styled(std::string_view t,
        color fg = color::reset,
        text_style st = text_style::normal) {
        return { t, fg, st };
    }

    // ---- ANSI 工具 ----
    inline constexpr std::string_view ansi_fg(color c) {
        using namespace std::string_view_literals;
        switch (c) {
        case color::black:   return "\033[30m"sv;
        case color::red:     return "\033[31m"sv;
        case color::green:   return "\033[32m"sv;
        case color::yellow:  return "\033[33m"sv;
        case color::blue:    return "\033[34m"sv;
        case color::magenta: return "\033[35m"sv;
        case color::cyan:    return "\033[36m"sv;
        case color::white:   return "\033[37m"sv;
        case color::orange:  return "\033[33m"sv;
        case color::reset:   return "\033[39m"sv;
        default:             return ""sv;
        }
    }

    inline constexpr std::string_view ansi_bg(color c) {
        using namespace std::string_view_literals;
        switch (c) {
        case color::black:   return "\033[40m"sv;
        case color::red:     return "\033[41m"sv;
        case color::green:   return "\033[42m"sv;
        case color::yellow:  return "\033[43m"sv;
        case color::blue:    return "\033[44m"sv;
        case color::magenta: return "\033[45m"sv;
        case color::cyan:    return "\033[46m"sv;
        case color::white:   return "\033[47m"sv;
        case color::orange:  return "\033[43m"sv;
        case color::reset:   return "\033[49m"sv;
        default:             return ""sv;
        }
    }

    inline void clear_screen() { std::print("\033[2J\033[1;1H"); }
    inline void clear_line() { std::print("\033[2K\r"); }

    // ---- 公共输出（使用 cprint/cprintln 避免与 std::print 重名） ----
    template <typename... Args>
    void cprint(std::format_string<Args...> fmt, Args&&... args) {
        std::print(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void cprintln(std::format_string<Args...> fmt, Args&&... args) {
        std::println(fmt, std::forward<Args>(args)...);
    }

    inline void themed_println(std::string_view text, color fg = color::reset,
        text_style st = text_style::normal) {
        cprintln("{}", styled(text, fg, st));
    }

    std::string prompt(std::string_view prompt_text, color c = color::cyan);
    void print_table(const std::vector<std::vector<std::string>>& data, bool header = false);

    // ---- 输入函数 ----
    std::string input(std::string_view prompt);
    std::string get_password(std::string_view prompt, char mask = '*');
    bool confirm(std::string_view prompt, bool default_yes = true);
    int menu_select(const std::vector<std::string>& items, std::string_view title = "");

    // ---- 随机与时间 ----
    void game_sleep(int milliseconds);
    std::mt19937& rng();
    bool chance(int percent);
    bool chance(double probability);

    // ---- 进度条 ----
    class progress_bar {
        int total_, width_;
        char fill_, empty_;
        bool show_percent_;
    public:
        progress_bar(int total, int width = 50, char fill = '=', char empty = ' ',
            bool show_percent = true);
        void update(int progress);
        void finish();
    };

    // ---- 旋转指示器 ----
    class spinner {
        std::jthread thread_;
        std::string message_;
    public:
        explicit spinner(std::string_view message = "");
        ~spinner();
        void start();
        void stop();
    };

    // ---- 日志 ----
    enum class log_level { info, warn, err };
    void log(log_level lvl, std::string_view msg);

} // namespace customio23

// ---- std::formatter 特化 styled_text ----
template <>
struct std::formatter<customio23::styled_text> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const customio23::styled_text& st, std::format_context& ctx) const {
        using namespace customio23;
        auto out = ctx.out();
        if (has_style(st.style, text_style::bold))      out = std::format_to(out, "\033[1m");
        if (has_style(st.style, text_style::italic))    out = std::format_to(out, "\033[3m");
        if (has_style(st.style, text_style::underline)) out = std::format_to(out, "\033[4m");

        color use_fg = adaptive_fg(st.fg);
        out = std::format_to(out, "{}", ansi_fg(use_fg));
        out = std::format_to(out, "{}", st.text);
        out = std::format_to(out, "\033[22m\033[23m\033[24m\033[39m");
        out = std::format_to(out, "{}", ansi_fg(adaptive_fg(current_theme.text)));
        return out;
    }
};

#endif // CUSTOMIO23_H