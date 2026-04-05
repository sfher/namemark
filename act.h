#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional> 
#include <memory>    

// If a macro named 'act' was defined by some other header it will break
// using 'act' as a class name (e.g. "std::unique_ptr<act>"). Undefine it
// here so the class declaration below is not affected.
#ifdef act
#undef act
#endif

class character;
struct FightContext;

// 行为基类
class act {
protected:
    std::unordered_map<std::string, int> consume_;   // 消耗属性映射
    int probability_;                                // 成功率（0-100）
    std::string name_;                               // 行为名称

    bool hasSufficientResources(const character* c) const;
    bool checkProbability() const;

public:
    act(int probability = 100) : probability_(probability) {}
    virtual ~act() = default;

    virtual bool can_execute(const character* c, const FightContext& ctx) const;
    virtual bool execute(character* c, FightContext& ctx) = 0;

    void set_consume(const std::string& name, int value) { consume_[name] = value; }
    std::string get_name() const { return name_; }
};

//技能工厂
struct SkillInfo {
    std::string name;
    int acquire_chance;
    int weight;
    std::function<std::unique_ptr<act>()> factory;
};

class SkillRegistry {
public:
    static void registerSkill(const std::string& id, const SkillInfo& info);
    static const SkillInfo* getSkillInfo(const std::string& id);
    static const std::unordered_map<std::string, SkillInfo>& getAllSkills();
private:
    static std::unordered_map<std::string, SkillInfo> skills_;
};

// 注册所有技能（在实现文件中定义）
void registerAllSkills();

// 攻击行为
class Attack : public act {
public:
    Attack();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};
//冲撞行为
class Smash : public act {
public:
    Smash();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};
//连击行为
class ComboAttack : public act {
public:
    ComboAttack();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};
//魔法飞弹
class MagicMissile : public act {
public:
    MagicMissile();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};
//加速魔法
class Speedup : public act {
public:
    Speedup();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

//毒魔法
class PoisonMagic : public act {
public:
    PoisonMagic();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

//火球术
class Fireball : public act {
public:
    Fireball();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

//治愈魔法
class HealMagic : public act {
public:
    HealMagic();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

//魔法重拳
class MagicPunch : public act {
public:
    MagicPunch();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

// 裂地术
class GroundFissure : public act {
public:
    GroundFissure();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

// 闪电术
class LightningChain : public act {
public:
    LightningChain();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

// 暴击判定器
class CritCalculator {
public:
    // 计算暴击后的伤害（如果暴击，返回暴击伤害；否则返回原伤害）
    static int apply(character* attacker, int base_damage);
};

// 防御行为
class Defense : public act {
public:
    Defense();
    bool can_execute(const character* c, const FightContext& ctx) const override;
    bool execute(character* c, FightContext& ctx) override;
};

// 目标选择器（独立工具）
class TargetSelector {
public:
    static character* select_lowest_hp(const std::vector<character*>& enemies);
};