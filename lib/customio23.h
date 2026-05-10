#ifndef CUSTOMIO23_H
#define CUSTOMIO23_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace customio23 {

    // ---- 颜色 ----
    enum class color : uint8_t {
        black, red, green, yellow, blue, magenta, cyan, white, reset, orange
    };

    // ---- 主题 ----
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
        .text       = color::white,
        .damage     = color::red,
        .heal       = color::green,
        .attack     = color::cyan,
        .special    = color::magenta,
        .warning    = color::yellow,
        .info       = color::blue,
        .error      = color::red,
        .prompt     = color::cyan,
        .title      = color::yellow
    };

    inline constexpr theme light_theme{
        .background = color::white,
        .text       = color::black,
        .damage     = color::red,
        .heal       = color::green,
        .attack     = color::blue,
        .special    = color::magenta,
        .warning    = color::yellow,
        .info       = color::blue,
        .error      = color::red,
        .prompt     = color::blue,
        .title      = color::black
    };

    extern theme current_theme;
    extern float g_battle_speed;

    void set_theme(const theme& t);
    const theme& get_theme();
    void apply_theme();
    color adaptive_fg(color desired);

    // ---- 样式 ----
    enum class text_style : uint8_t {
        normal    = 0,
        bold      = 1 << 0,
        italic    = 1 << 1,
        underline = 1 << 2
    };

    constexpr text_style operator|(text_style a, text_style b) {
        return static_cast<text_style>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    constexpr bool has_style(text_style s, text_style flag) {
        return (static_cast<uint8_t>(s) & static_cast<uint8_t>(flag)) != 0;
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

    std::ostream& operator<<(std::ostream& os, const styled_text& st);

    // ---- ANSI ----
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

    inline void clear_screen() { std::cout << "\033[2J\033[1;1H"; }
    inline void clear_line()   { std::cout << "\033[2K\r"; }

    // ---- cprint / cprintln（C++17 兼容，{} 替换实现）----
    namespace detail {
        template<typename T>
        void write_one(std::ostream& os, const T& v) { os << v; }

        inline void write_fmt(std::ostream& os, std::string_view fmt) {
            os << fmt;  // output remaining literal text
        }

        template<typename T, typename... Rest>
        void write_fmt(std::ostream& os, std::string_view fmt, const T& first, Rest&&... rest) {
            auto pos = fmt.find("{}");
            if (pos != std::string_view::npos) {
                os << fmt.substr(0, pos);
                write_one(os, first);
                write_fmt(os, fmt.substr(pos + 2), std::forward<Rest>(rest)...);
            }
        }
    } // namespace detail

    template<typename... Args>
    void cprint(std::string_view fmt, Args&&... args) {
        detail::write_fmt(std::cout, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void cprintln(std::string_view fmt, Args&&... args) {
        cprint(fmt, std::forward<Args>(args)...);
        std::cout << '\n';
    }

    inline void themed_println(std::string_view text, color fg = color::reset,
                                text_style st = text_style::normal) {
        cprintln("{}", styled(text, fg, st));
    }

    // ---- 输入 ----
    std::string input(std::string_view prompt);
    std::string get_password(std::string_view prompt, char mask = '*');
    bool confirm(std::string_view prompt, bool default_yes = true);
    int menu_select(const std::vector<std::string>& items, std::string_view title = "");

    // ---- 输出 ----
    void print_table(const std::vector<std::vector<std::string>>& data, bool header = false);

    // ---- 时间 / 随机 ----
    void game_sleep(int ms);
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

    // ---- 旋转器（C++17：std::thread + atomic）----
    class spinner {
        std::thread thread_;
        std::atomic<bool> running_{false};
        std::string message_;
        void run();
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

#endif
