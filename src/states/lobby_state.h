// states/lobby_state.h
#pragma once

#include "../game.h"

class LobbyState : public GameState {
public:
    explicit LobbyState(GameContext& ctx);
    void on_enter() override;
    void update() override;
};