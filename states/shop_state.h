// states/shop_state.h
#pragma once

#include "../game.h"
#include <vector>

class ShopState : public GameState {
public:
    // 使用 explicit 防止隐式转换，保持良好风格
    explicit ShopState(GameContext& ctx);

    // [[nodiscard]] 提醒调用者不应忽略返回值（虽然这里是 void，但可作为基类虚函数标记）
    // 现代 C++ 建议对“无异常安全”的函数使用 noexcept 如果确实不会抛出异常
    void on_enter() override;
    void update() override;

private:
    // 使用 const 引用传递较大对象（武器模板）避免拷贝
    // 函数名更清晰地表达意图
    void show_weapon_list();

    // 传参时使用 const 引用，表明不会修改武器模板
    void buy_weapon(const WeaponData& weapon_template);

    // 同理，const 引用传递武器模板
    void show_character_select_for_weapon(const WeaponData& weapon_template);
};