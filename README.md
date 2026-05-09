# Namemark

回合制 RPG 文字战斗游戏，支持技能自定义、模组系统、角色养成与抽卡。

## 快速开始

### 编译运行

```bash
g++ -std=c++17 ./*.cpp states/*.cpp -o main
./main
```

支持 Linux / macOS / Windows（MSVC 或 MinGW）。

### 基本操作

| 按键 | 场景 | 功能 |
|------|------|------|
| ↑ ↓ | 所有菜单 | 移动光标 / 选择上一项下一项 |
| Enter | 所有菜单 | 确认当前选项 |
| Space | 多选列表 | 勾选 / 取消勾选当前项 |
| E | 角色详情 | 装备 / 卸下武器 |
| D | 角色详情 | 运行战斗评分测试 |
| Esc | 角色详情 | 返回上级菜单 |
| 任意键 | 消息提示后 | 继续 |

## 游戏流程

启动后首先选择模组包（`packages/` 目录），随后进入大厅：

- **队伍管理** — 召唤角色、编排出战队伍（最多5人）、查看角色详情、装备武器、战斗评分测试
- **队伍测试** — 两支队伍间的友谊演习对战
- **开始冒险** — PvE 关卡挑战，线性解锁
- **神秘商店** — 消耗金币购买武器
- **抽卡模式** — 单抽 / 十连抽获取角色与武器
- **设置模式** — 调整战斗速度倍率（0.5x – 2.0x）
- **控制台** — 开发者调试终端（输入 `/help` 查看命令列表）

---

# 配置编辑指南

欢迎来到 Namemark 策划配置文档！仅需修改 JSON 配置文件，重启游戏即可实时生效。

## 一、文件说明

| 文件路径 | 核心作用 | 是否需要修改代码 |
|----------|----------|:------------------:|
| `monsters.json` | 定义全部怪物的基础属性、战斗技能组合 | 否 |
| `levels.json` | 定义全部关卡信息、剧情描述、敌方怪物组合 | 否 |
| `config/skills/acts.json` | 自定义新增/修改战斗技能 | 否 |

**重要规范：** 所有配置文件必须保存为 UTF-8 无 BOM 编码，推荐使用 VS Code、Notepad++、Sublime Text 等专业编辑器。

## 二、怪物配置（monsters.json）

```json
{
  "monsters": [
    {
      "id": "slime",
      "name": "史莱姆",
      "attributes": { "HP": 120, "MAX_HP": 120, "ATK": 30, "DEF": 15 },
      "actions": ["attack", "smash"]
    }
  ]
}
```

### 属性字段

| 字段 | 说明 | 推荐范围 |
|------|------|----------|
| HP / MAX_HP | 当前/最大生命值 | 80–400 |
| MP | 魔法值 | 0–120 |
| ATK | 物理攻击力 | 20–80 |
| DEF | 物理防御力 | 10–50 |
| MATK | 魔法攻击力 | 10–80 |
| SPD | 行动速度 | 10–50 |
| CRIT | 暴击几率（%） | 5–30 |
| CRIT_D | 暴击伤害倍率（150=1.5倍） | 150–200 |
| C_AP | 初始行动点 | 20–55 |

### 可用技能ID

**基础：** `attack`（普通攻击）、`defense`（防御姿态，减伤50%）

**物理：** `smash`（冲撞）、`combo`（连击×3）、`heavy_strike`（重伤打击）、`lifesteal`（吸血攻击）

**魔法：** `magic_missile`（魔法飞弹）、`fireball`（火球术+燃烧）、`poison_magic`（毒魔法）、`lightning`（闪电弹射×3）、`void_explosion`（虚爆）、`meteor_strike`（陨石全屏）、`ground_fissure`（裂地术）、`magic_punch`（魔拳混合）

**治疗增益：** `heal_magic`（治愈）、`self_heal`（自愈）、`speedup`（加速）、`stone_skin`（石肤减伤70%）、`meditate`（冥想回蓝）

**召唤：** `summon_phantom`（召唤幻影）、`void_god_summon`（虚神召唤）

### 怪物平衡建议

| 定位 | 生命值 | 攻防 | 技能数 |
|------|--------|------|--------|
| 普通小怪 | 80–160 | 25–45 | 1~2 |
| 精英怪物 | 180–280 | 45–65 | 2~3 |
| BOSS | 300–400 | 65–80 | 3~4（必带群体/爆发技） |

## 三、关卡配置（levels.json）

```json
{
  "levels": [
    {
      "id": "forest_1",
      "name": "初出茅庐",
      "description": "两个史莱姆阻挡前路，轻松即可应对。",
      "max_allies": 2,
      "enemies": [
        { "preset_id": "slime" },
        { "preset_id": "slime" }
      ]
    }
  ]
}
```

### 字段说明

| 字段 | 说明 |
|------|------|
| id | 关卡唯一编号，英文小写 |
| name | 关卡显示名称 |
| description | 战前剧情描述 |
| max_allies | 最大出战角色数 |
| enemies[].preset_id | 引用 monsters.json 中的怪物ID |

可单独为关卡内怪物临时覆盖属性：`{ "preset_id": "slime", "attributes": { "HP": 120 } }`

### 关卡梯度参考

| 阶段 | 敌方数量 | 定位 | 总血量 | 推荐出战 |
|------|----------|------|--------|----------|
| 1–3关 | 1~2只 | 基础小怪 | 160–320 | 2–3人 |
| 4–6关 | 2~3只 | 小怪+精英 | 350–550 | 3人 |
| 7–9关 | 3~4只 | 精英为主 | 500–800 | 4人 |
| 10关以上 | 4~5只 | BOSS+杂兵 | 800–1200 | 4–5人 |

## 四、技能配置（config/skills/acts.json）

```json
{
  "skills": [
    {
      "id": "my_skill",
      "display_name": "我的技能",
      "description": "自定义技能描述",
      "acquire_chance": 40,
      "weight": 7,
      "base_probability": 80,
      "consume": { "MP": 20, "C_AP": 15 },
      "target_type": "SINGLE_ENEMY_LOWEST_HP",
      "executor_type": "SINGLE_DAMAGE",
      "hit_rate": 85,
      "dmg_formula": "ATK",
      "dmg_coeff": 2.0,
      "use_def": true
    }
  ]
}
```

### 目标类型

| 参数 | 说明 |
|------|------|
| SINGLE_ENEMY_LOWEST_HP | 血量最低敌方 |
| SINGLE_ENEMY_RANDOM | 随机敌方 |
| SINGLE_ENEMY_HIGHEST_ATK | 攻击最高敌方 |
| SINGLE_ALLY_LOWEST_HP | 血量最低队友 |
| SINGLE_ALLY_SELF | 自身 |
| ALL_ENEMIES | 敌方全体 |
| ALL_ALLIES | 我方全体 |
| NONE | 无目标（纯增益） |

### 执行器类型

| 参数 | 说明 |
|------|------|
| SINGLE_DAMAGE | 单体伤害 |
| AOE_DAMAGE | 群体范围伤害 |
| SELF_BUFF | 自身增益 |

### 伤害公式

| 公式 | 说明 |
|------|------|
| ATK | 纯物理攻击 |
| MATK | 纯魔法攻击 |
| ATK_MATK_SUM | 物攻+魔攻直加 |
| ATK_MATK_COMBINED | 双属性总和×系数 |
| ATK_POWER | 物理指数增幅 |
| MATK_POWER | 魔法指数增幅 |

### 可用 Buff 效果

| Buff | 效果 | 参数 |
|------|------|------|
| DEFENSE_BUFF | 伤害减免护盾 | 减伤% + 持续回合 |
| POISON | 中毒持续伤害 | 每回合伤害 + 持续回合 |
| BURN | 燃烧持续伤害 | 每回合伤害 + 持续回合 |
| SELF_HEAL | 持续生命回复 | 每回合回复量 + 持续回合 |
| SPD_BUFF | 速度行动增益 | 额外行动点 + 持续回合 |

### 技能制作速查

| 类型 | 执行器 | 目标类型 | 推荐消耗 | 伤害系数 |
|------|--------|----------|----------|----------|
| 单体物攻 | SINGLE_DAMAGE | 最低血量敌人 | C_AP: 15–30 | 1.2–2.5 |
| 单体法攻 | SINGLE_DAMAGE | 最低血量敌人 | MP + C_AP | 1.5–2.5 |
| 群体攻击 | AOE_DAMAGE | 全体敌人 | 高消耗 | 1.2–3.0 |
| 自身增益 | SELF_BUFF | NONE | 中等消耗 | 无 |

## 五、控制台调试命令

游戏内进入控制台后，输入 `/help` 可查看完整命令列表：

| 命令 | 说明 |
|------|------|
| `@TeamName` | 创建队伍 |
| `CharName @TeamName` | 创建角色并编入队伍 |
| `/fight` | 所有队伍对战 |
| `/test CharName` | 查看角色详细属性 |
| `/ftest [CharName]` | 快速静态评分 |
| `/simulate A vs B [回合]` | 批量战斗模拟 |
| `/find [数量]` | 查找有加护的角色 |
| `/theme [名称]` | 切换控制台主题 |
| `/settheme` | 交互式主题选择器 |
| `/import <路径>` | 从 JSON 导入角色 |
| `/hfind` | 高分角色搜索 |
| `/end` | 退出控制台 |

可用主题：`default`（黑底白字）、`light`（白底黑字）、`dark`（黑底绿字）、`retro`（琥珀色）、`highcontrast`（黄底黑字高对比）。

## 六、常见问题

**Q：新增怪物/技能后不生效？** A：检查 JSON 语法（逗号、引号、括号是否配对）、ID 唯一性、文件编码为 UTF-8 无 BOM，修改后重启游戏。

**Q：配置中文出现乱码？** A：禁用系统记事本，使用 VS Code 等专业编辑器，UTF-8 无 BOM 保存。

**Q：怪物只普通攻击不释放技能？** A：核对 `actions` 内技能 ID 大小写和拼写，须与官方技能列表完全一致。

## 七、项目结构

```
namemark/
├── packages/             # 模组包目录
├── config/skills/        # 技能配置文件
├── states/               # 游戏状态（大厅、队伍、冒险、商店、抽卡等）
├── monsters.json         # 怪物预设
├── levels.json           # 关卡配置
├── weapons.json          # 武器模板
├── customio.h/cpp        # 跨平台终端 I/O 库
├── customio23.h/cpp      # C++17 兼容终端 I/O 库（备选）
└── namemark_version1.4.cpp  # 入口点
```
