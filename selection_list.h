// selection_list.h
#pragma once

#include <vector>
#include <string>
#include <functional>
#include <conio.h>
#include "customio.h"

// 通用多选列表组件
class SelectionList {
public:
    // 渲染单个项目的回调：参数为项目索引、是否选中、是否高亮（光标所在）
    using RenderItemFunc = std::function<void(size_t index, bool selected, bool highlighted)>;

    SelectionList() = default;

    // 配置列表
    void set_item_count(size_t count) { item_count_ = count; selected_.resize(count, false); }
    void set_max_selection(size_t max_sel) { max_selection_ = max_sel; }
    void set_page_size(size_t page_size) { page_size_ = page_size; }

    // 运行交互，返回最终选中的索引列表
    std::vector<size_t> run(RenderItemFunc render_func);

    // 预置选中状态
    void set_selected(size_t index, bool val) { if (index < selected_.size()) selected_[index] = val; }
    bool is_selected(size_t index) const { return index < selected_.size() && selected_[index]; }

private:
    void render_page(RenderItemFunc render_func);
    void clear_rendered_lines(size_t lines);

    size_t item_count_ = 0;
    size_t max_selection_ = 4;
    size_t page_size_ = 10;
    size_t cursor_index_ = 0;
    std::vector<bool> selected_;
};