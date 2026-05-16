// selection_list.cpp
#include "selection_list.h"
#include <iostream>
#ifdef _WIN32
#include <conio.h>
#endif

using namespace customio;

std::vector<size_t> SelectionList::run(RenderItemFunc render_func) {
    cursor_index_ = 0;
    bool running = true;

    clear_screen();
    render_page(render_func);

    flush_stdin();
    while (running) {
#ifdef _WIN32
        int ch = _getch();
        if (ch == 224) {
            ch = _getch();
            size_t old_cursor = cursor_index_;
            if (ch == 72) cursor_index_ = (cursor_index_ - 1 + item_count_) % item_count_;
            else if (ch == 80) cursor_index_ = (cursor_index_ + 1) % item_count_;

            if (old_cursor != cursor_index_) {
                render_page(render_func);
            }
        } else if (ch == 13 || ch == 10) {
            running = false;
        } else if (ch == 27) {
            // ESC — return empty selection
            selected_.assign(item_count_, false);
            running = false;
        } else if (ch >= '1' && ch <= '9') {
            size_t idx = ch - '1';
            if (idx < item_count_ && idx != cursor_index_) {
                cursor_index_ = idx;
                render_page(render_func);
            }
        } else if (ch == 32) { // Space
            goto handle_space;
        }
#else
        int ch = getch();
        if (ch == '\x1b') {
            int c2 = getch();
            if (c2 == -1) {
                // Bare ESC
                selected_.assign(item_count_, false);
                running = false;
            } else if (c2 == '[') {
                int c3 = getch();
                size_t old_cursor = cursor_index_;
                if (c3 == 'A') cursor_index_ = (cursor_index_ - 1 + item_count_) % item_count_;
                else if (c3 == 'B') cursor_index_ = (cursor_index_ + 1) % item_count_;

                if (old_cursor != cursor_index_) {
                    render_page(render_func);
                }
            }
        } else if (ch == '\n' || ch == '\r') {
            running = false;
        } else if (ch >= '1' && ch <= '9') {
            size_t idx = ch - '1';
            if (idx < item_count_ && idx != cursor_index_) {
                cursor_index_ = idx;
                render_page(render_func);
            }
        } else if (ch == 32) { // Space
            goto handle_space;
        }
        continue; // skip the handle_space block at bottom
#endif
        if (false) {
        handle_space:
            if (cursor_index_ < item_count_) {
                bool currently_selected = selected_[cursor_index_];
                if (!currently_selected) {
                    size_t selected_count = 0;
                    for (bool s : selected_) selected_count += s;
                    if (selected_count >= max_selection_)
                        continue;
                }
                if (!currently_selected) {
                    if (max_selection_ == 1) {
                        for (size_t i = 0; i < selected_.size(); ++i)
                            selected_[i] = false;
                    } else {
                        size_t selected_count = 0;
                        for (bool s : selected_) selected_count += s;
                        if (selected_count >= max_selection_) continue;
                    }
                }
                selected_[cursor_index_] = !currently_selected;
                save_cursor();
                move_up(static_cast<int>(cursor_index_ % page_size_));
                clear_line();
                render_func(cursor_index_, selected_[cursor_index_], true);
                restore_cursor();
            }
        }
    }

    std::vector<size_t> result;
    for (size_t i = 0; i < selected_.size(); ++i) {
        if (selected_[i]) result.push_back(i);
    }
    return result;
}

void SelectionList::render_page(RenderItemFunc render_func) {
    size_t page_start = (cursor_index_ / page_size_) * page_size_;
    size_t page_end = std::min(page_start + page_size_, item_count_);

    clear_screen();

    for (size_t i = page_start; i < page_end; ++i) {
        render_func(i, selected_[i], i == cursor_index_);
        std::cout << std::endl;
    }

    const auto& theme = get_console_theme();
    std::cout << "\n-----------------------------------\n";
    std::cout << "↑↓: 移动  Space: 选择  Enter: 确认  Esc: 返回  数字: 直达\n";
    std::cout << "当前选中: ";
    size_t selected_count = 0;
    for (bool s : selected_) selected_count += s;
    std::cout << selected_count << " / " << max_selection_;
    if (selected_count >= max_selection_) {
        std::cout << adaptive_textcolor(theme.warning) << " (已达上限)" << resetcolor();
    }
    std::cout << std::endl;
}

void SelectionList::clear_rendered_lines(size_t lines) {
    for (size_t i = 0; i < lines; ++i) {
        move_up(1);
        clear_line();
    }
}
