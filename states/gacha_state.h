// states/gacha_state.h
#pragma once
#include "../game.h"

struct PullResult {
    std::string type;   // "character" 或 "weapon"
    std::string id;     // preset_id 或 weapon_id
    std::string rarity; // "SSR", "SR", "R"
};

class GachaState : public GameState {
public:
    explicit GachaState(GameContext& ctx);
    void update() override;

private:
    void single_pull();
    void ten_pull();
    PullResult random_pull();   // 返回 PullResult
    void display_result(const std::vector<PullResult>& results);
};