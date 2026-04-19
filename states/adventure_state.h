// states/adventure_state.h
#pragma once

#include "../game.h"

class AdventureState : public GameState {
public:
    explicit AdventureState(GameContext& ctx);
    void on_enter() override;
    void update() override;

private:
    bool play_level(int level_index);
};