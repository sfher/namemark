// states/settings_state.h
#pragma once
#include "../game.h"

class SettingsState : public GameState {
public:
    explicit SettingsState(GameContext& ctx);
    void on_enter() override;
    void update() override;
};