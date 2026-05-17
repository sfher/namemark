// selection_list.h — 通用多选列表组件
// 用于出战选择、队伍调整等场景；支持单选/多选、分页、光标循环
#pragma once

#include <vector>
#include <string>
#include <functional>
#include "customio.h"

class SelectionList {
public:
    // 渲染单项回调：index / 是否选中 / 是否高亮
    using RenderItemFunc = std::function<void(size_t index, bool selected, bool highlighted)>;

    SelectionList() = default;

    void set_item_count(size_t count) { item_count_ = count; selected_.resize(count, false); }
    void set_max_selection(size_t max_sel) { max_selection_ = max_sel; }
    void set_page_size(size_t page_size) { page_size_ = page_size; }

    // 启动交互循环，返回选中索引列表（ESC 返回空列表）
    std::vector<size_t> run(RenderItemFunc render_func);

    void set_selected(size_t index, bool val) { if (index < selected_.size()) selected_[index] = val; }
    bool is_selected(size_t index) const { return index < selected_.size() && selected_[index]; }

private:
    void render_page(RenderItemFunc render_func);
    void clear_rendered_lines(size_t lines);

    size_t item_count_ = 0;
    size_t max_selection_ = 4;     // 多选上限，1 = 单选
    size_t page_size_ = 10;
    size_t cursor_index_ = 0;
    std::vector<bool> selected_;
};
