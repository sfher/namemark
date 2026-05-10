// states/teamtest_state.h
#pragma once
#include "../game.h"
#include "../selection_list.h"

class TeamTestState : public GameState {
public:
    explicit TeamTestState(GameContext& ctx);
    void on_enter() override;
    void update() override;

private:
    void select_team(const std::string& title, size_t max_selection, std::vector<size_t>& result);
    void start_exercise();
    void print_exercise_report(Team& teamA, Team& teamB);

    std::unique_ptr<SelectionList> selector_;
};