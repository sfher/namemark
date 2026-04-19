// selection_list.cpp
#include "selection_list.h"
#include <iostream>

using namespace customio;

std::vector<size_t> SelectionList::run(RenderItemFunc render_func) {
    cursor_index_ = 0;
    bool running = true;

    // 首次渲染
    clear_screen();
    render_page(render_func);

    while (running) {
        int ch = _getch();
        if (ch == 224) { // 方向键
            ch = _getch();
            size_t old_cursor = cursor_index_;
            if (ch == 72 && cursor_index_ > 0) { // 上
                cursor_index_--;
            }
            else if (ch == 80 && cursor_index_ < item_count_ - 1) { // 下
                cursor_index_++;
            }

            if (old_cursor != cursor_index_) {
                // 重新绘制当前页（光标位置改变）
                render_page(render_func);
            }
        }
        else if (ch == 32) { // 空格键
            if (cursor_index_ < item_count_) {
                bool currently_selected = selected_[cursor_index_];
                if (!currently_selected) {
                    // 检查是否已达上限
                    size_t selected_count = 0;
                    for (bool s : selected_) selected_count += s;
                    if (selected_count >= max_selection_) {
                        // 已达上限，无法勾选
                        // 可以在此处输出短暂提示（可选）
                        continue;
                    }
                }
                // 在空格键处理中
                if (!currently_selected) {
                    if (max_selection_ == 1) {
                        // 单选模式：清空其他选中
                        for (size_t i = 0; i < selected_.size(); ++i) {
                            selected_[i] = false;
                        }
                    }
                    else {
                        // 多选模式：检查上限
                        size_t selected_count = 0;
                        for (bool s : selected_) selected_count += s;
                        if (selected_count >= max_selection_) continue;
                    }
                }
                selected_[cursor_index_] = !currently_selected;
                // 重绘当前项（就地刷新单行）
                save_cursor();
                move_up(static_cast<int>(cursor_index_ % page_size_));
                clear_line();
                render_func(cursor_index_, selected_[cursor_index_], true);
                restore_cursor();
            }
        }
        else if (ch == 13) { // 回车键
            running = false;
        }
    }

    // 收集选中的索引
    std::vector<size_t> result;
    for (size_t i = 0; i < selected_.size(); ++i) {
        if (selected_[i]) result.push_back(i);
    }
    return result;
}

void SelectionList::render_page(RenderItemFunc render_func) {
    // 计算当前页起始索引
    size_t page_start = (cursor_index_ / page_size_) * page_size_;
    size_t page_end = std::min(page_start + page_size_, item_count_);

    // 清除之前渲染的区域（简单起见，清屏重绘，后续可优化为局部刷新）
    clear_screen();

    // 绘制标题行（由外部调用者提前输出）
    // 绘制列表项
    for (size_t i = page_start; i < page_end; ++i) {
        render_func(i, selected_[i], i == cursor_index_);
        std::cout << std::endl;
    }

    // 绘制操作提示
    const auto& theme = get_console_theme();
    std::cout << "\n-----------------------------------\n";
    std::cout << "↑↓: 移动  Space: 选择  Enter: 确认\n";
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