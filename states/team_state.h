// states/team_state.h
#pragma once

#include "../game.h"
#include "../selection_list.h"

class TeamState : public GameState {
public:
    explicit TeamState(GameContext& ctx);
    void on_enter() override;
    void update() override;

private:
    void add_new_character();
    void select_team();
    void list_characters();

    std::unique_ptr<SelectionList> selection_list_;
};