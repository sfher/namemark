#namemark_version1.4
一个高中生对 namerena 项目的拙劣模仿 —— 为了熟悉《游戏编程模式》中的各种设计范式而写的小型回合制战斗模拟器。

项目简介
这是一个用 C++20 编写的控制台回合制战斗游戏框架。角色拥有 HP、MP、ATK、DEF、SPD 等属性，可以使用多种技能（攻击、防御、魔法、召唤等）进行战斗。项目实现了完整的战斗循环、技能系统、buff/debuff、队伍管理，并提供了一个交互式 Debug Console 用于测试和模拟对战。

主要特点：(来自deepseek概括)
基于 AP（行动点）的回合制战斗
技能随机学习机制（每个角色生成时根据哈希值随机获得技能）
支持技能注册表与工厂模式，方便扩展新技能
多队伍混战，自动索敌
导入/导出角色（JSON 格式）
控制台彩色输出、进度条、打字机效果等交互增强

项目结构
text
namemark_version1.4/
├── act.h / act.cpp          # 技能（行为）系统
├── entity.h / entity.cpp    # 角色、队伍、战斗引擎
├── customio.h / customio.cpp # 控制台美化工具
├── console.h / console.cpp  # Debug Console 交互逻辑
├── namemark_version1.4.cpp  # 程序入口
└── test_characters.json     # 示例角色数据
尝试设计的模块：
1. 技能（行动）系统
act 基类：定义技能的通用接口（can_execute, execute, 消耗资源, 目标类型等）。
派生技能：Attack, Smash, Fireball, HealMagic…… 每个技能封装自己的行为逻辑。
SkillRegistry：全局注册表，以字符串 ID 映射到 SkillInfo（包含技能名称、学习权重、获取概率、工厂函数）。
工厂函数：std::function<std::unique_ptr<act>()>，用于在角色生成时动态创建技能实例。

2. 角色生成
角色名字通过哈希值决定基础属性（HP, ATK 等）和是否获得“加护”（中二病日常233）。
技能获取：遍历技能注册表，根据每个技能的 acquire_chance 掷骰决定是否加入角色的动作列表。
setbasicattr() 和 setaegis() 分离了属性初始化与特殊能力附加，类似于组件模式（？）。

3. 战斗系统 – 优先级队列 + 上下文对象
FightComponent 维护一个最大堆（priority_queue<ActionNode>），按当前 AP 排序决定行动顺序。
每回合取出 AP 最高的角色，调用 do_action()，扣减 AP，若角色仍存活则重新入队。
全局 AP 恢复：当所有角色 AP 都不足最小消耗时，调用 recover_ap() 为每个存活角色增加 SPD 值。
FightContext 作为上下文对象，传递给每个技能，包含敌我列表、队伍映射、召唤物列表等。
这是命令模式（技能作为命令对象）与上下文对象模式的结合。战斗引擎不关心具体技能实现，只负责调度和执行命令。

4. Debug Console – 命令模式
实际手感比较像命令行
支持创建队伍、添加角色、导入 JSON、批量模拟战斗，方便调试和平衡性测试。

5. 角色导入系统 – 简单解析 + 外部数据驱动
手动解析 JSON（没有依赖第三方库），读取 characters 数组，存入 imported_characters 映射。

构造角色时检测名字是否为 [:id] 格式，若匹配则从导入数据中读取属性，覆盖默认生成逻辑。

这是一种数据驱动设计的尝试：角色数据从外部文件加载，无需重新编译即可调整平衡性。

6. Buff 系统 – 组件模式
character 内部维护 buffs_ 映射（buff名 -> 效果值, 剩余回合）。
apply_buffs() 在每回合行动前触发效果（中毒、燃烧、防御增益等）。
update_buffs() 在行动后减少持续时间并清除过期 buff。
虽然没有完全解耦为独立的 Buff 组件，但已经将状态效果从技能逻辑中分离，便于扩展。

总结
虽然是个玩玩的项目，但还是挺有收获的。vibecode之余也巩固了一下c++基础（）

experience：

工厂 + 注册表让技能扩展变得极其简单（新增技能只需写一个类 + 一行注册）。

namerena中的行动模拟很难，我用priority进行了拙劣模仿。

数据驱动设计对游戏平衡调整非常有帮助。

未来可能的改进方向：使用真正的 JSON 库（如 nlohmann/json），抽象出独立的策略类，增加事件系统，重构 Buff 为独立组件。

最后更新：2026 年 4 月
