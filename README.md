# MiniSpireSFML

一款基于 C++ + SFML 的类《杀戮尖塔》卡牌 Roguelike 游戏。

## 项目概述

MiniSpireSFML 是一款单机卡牌构建游戏（Deckbuilding Roguelike），灵感来源于经典游戏《杀戮尖塔》。玩家需要通过策略性地使用卡牌、收集遗物、管理资源，穿越 15 层高塔，最终击败尖塔守卫。

### 核心玩法

- **卡牌战斗**：每回合使用能量打出卡牌，造成伤害或获得格挡
- **牌组构建**：通过战斗奖励获取新卡牌，打造专属牌组
- **遗物收集**：击败精英敌人获得强力遗物，获得被动增益
- **资源管理**：合理分配金币、药剂和生命值

## 功能特性

### 卡牌系统

- **38+ 张可玩卡牌**，分为三种类型：
  - 攻击牌（红色）：造成伤害
  - 技能牌（蓝色）：获得格挡或效果
  - 能力牌（黄色）：持续增益效果
- **卡牌升级**：营火锻造可将卡牌升级为增强版
- **卡牌消耗**：部分强力卡牌打出后消耗，本场战斗不再可用

### 敌人系统

| 类型 | 数量 | 特点 |
|------|------|------|
| 普通敌人 | 4 种 | 基础战斗，热身阶段 |
| 精英敌人 | 3 种 | 高难度，掉落遗物 |
| BOSS | 1 种 | 最终挑战，180 HP |

### 遗物系统

| 遗物名称 | 效果描述 |
|----------|----------|
| 余烬护符 | 首回合开始时额外获得 1 点能量 |
| 铁躯纹章 | 获得 +8 最大生命值并回复 8 点生命 |
| 血瓶 | 每场战斗开始时回复 5 点生命 |
| 战术透镜 | 每回合额外抽 1 张牌 |
| 荆棘徽记 | 受到攻击时反伤 3 点伤害 |

### 其他特性

- 完整的中文界面和卡牌描述
- 轻量级动画效果（可配置开关）
- 浮动文字战斗反馈
- 配置文件支持（帧率、动画、音量等）
- 通关次数持久化存储

## 安装指南

### 系统要求

- Windows 10/11 或 Linux 或 macOS
- 支持 OpenGL 2.1 的显卡

### 从源码构建

#### 前置依赖

- CMake 3.16+
- Ninja 构建工具
- C++17 兼容编译器（g++、clang++ 或 MSVC）

#### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/FujimiyaKaor1/MiniSpire.git
cd MiniSpire

# 配置项目
cmake --preset default

# 构建
cmake --build --preset default

# 运行
./build/MiniSpireSFML
```

#### 启用音频（可选）

默认关闭音频以提高稳定性。如需启用：

```bash
cmake --preset default -DENABLE_AUDIO=ON
cmake --build --preset default
```

### 直接运行

Windows 用户可直接双击 `MiniSpireSFML.exe` 运行游戏。

## 使用说明

### 游戏控制

| 操作 | 说明 |
|------|------|
| 鼠标点击 | 选择卡牌、按钮、选项 |
| R 键 | 重新开始本局游戏 |
| T 键 | 开关战斗动画 |
| M 键 | 切换静音/开启音频 |
| +/- 键 | 调节音量 |

### 游戏流程

```
第 1-2 层  → 普通战斗（热身）
第 3 层    → 营火休息（回复生命/锻造升级）
第 4-5 层  → 战斗 + 精英敌人
第 6 层    → 商店（购买卡牌/药剂/删除卡牌）
第 7-8 层  → 战斗 + 随机事件
第 9 层    → 营火休息
第 10-12 层 → 精英战斗（难度提升）
第 13 层   → 随机事件
第 14 层   → 营火休息（最终准备）
第 15 层   → BOSS 战 - 尖塔守卫
```

### 战斗机制

1. **回合开始**：抽 5 张牌，获得 3 点能量
2. **出牌阶段**：点击卡牌打出（需满足能量消耗）
3. **结束回合**：手牌进入弃牌堆，敌人执行意图
4. **状态效果**：力量增伤、易伤/虚弱每回合递减

### 配置文件

首次运行会生成 `game_config.ini`：

```ini
# MiniSpireSFML runtime config
max_fps=60
enable_animations=true
enable_floating_text=true
text_scale=1.0
```

## 项目结构

```
MiniSpireSFML/
├── src/
│   └── main.cpp          # 游戏主逻辑（单文件架构）
├── assets/
│   ├── audio/            # 音频资源
│   │   ├── bgm/          # 背景音乐
│   │   └── sfx/          # 音效
│   ├── fonts/            # 字体文件
│   └── images/           # 图像资源
│       ├── backgrounds/  # 场景背景
│       ├── cards/        # 卡牌背景
│       ├── enemies/      # 敌人立绘
│       └── ui/           # UI 元素
├── CMakeLists.txt        # CMake 配置
├── game_config.ini       # 运行时配置
└── README.md             # 项目文档
```

## 技术架构

### 核心设计

- **单文件架构**：所有游戏逻辑集中在 `main.cpp`（约 4500 行）
- **状态机模式**：通过 `Phase` 枚举控制游戏流程
- **数据驱动**：卡牌、敌人、遗物等数据结构化定义

### 主要系统

- **卡牌战斗系统**：支持攻击、技能、能力三种卡牌类型
- **敌人 AI 系统**：基于意图模式的敌人行为
- **遗物系统**：提供被动增益效果
- **事件系统**：随机事件增加游戏变数
- **商店系统**：购买卡牌、遗物、药水

### 技术栈

- **语言**：C++17
- **图形库**：SFML 2.6
- **构建系统**：CMake + Ninja
- **随机数**：Mersenne Twister (std::mt19937)

## 贡献规范

欢迎提交 Issue 和 Pull Request！

### 开发指南

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 代码规范

- 遵循现有代码风格
- 添加必要的注释
- 确保通过现有测试

## 许可证

本项目仅供学习和研究使用。

### 第三方资源

- 字体：Noto Sans CJK、ZCOOL 快乐体（Apache 2.0 / SIL OFL）
- 图标：OpenMoji（CC BY-SA 4.0）
- 音频：原创或开源素材

详见 `assets/THIRD_PARTY_LICENSES.md`

## 致谢

- 灵感来源：《杀戮尖塔》(Slay the Spire)
- 图形框架：SFML
- 开源社区贡献者

---

**作者**：FujimiyaKaor1

**项目地址**：[https://github.com/FujimiyaKaor1/MiniSpire](https://github.com/FujimiyaKaor1/MiniSpire)
