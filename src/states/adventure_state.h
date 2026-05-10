// states/adventure_state.h
#pragma once

#include "../game.h"
#include "../selection_list.h"

class AdventureState : public GameState {
public:
    explicit AdventureState(GameContext& ctx);
    void on_enter() override;
    void update() override;

private:
    bool play_level(int level_index);
    bool adjust_team_for_level(int max_allies);  //调整队伍人数
	void show_level_list_menu();   // 显示关卡列表并处理选择
};

void print_fight_report(Team& teamA, Team& teamB);  // 战斗结算报表函数