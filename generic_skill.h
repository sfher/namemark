// generic_skill.h
#pragma once

#include "act.h"
#include "skill_data.h"

class GenericSkill : public act {
public:
    explicit GenericSkill(const SkillData& data);

    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;

private:
    SkillData data_;
};