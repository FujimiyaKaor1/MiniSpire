# MiniSpireSFML

一个基于 C++ + SFML 的类《杀戮尖塔》最小可玩 Demo。

当前阶段：可演示版本（已接入联网开源素材并中文化）。

Implemented scope:
- 1 名可玩角色（铁甲战士风格）
- 20 张战士卡牌
- 3 种普通敌人
- 1 种精英敌人
- 1 个 BOSS
- 固定 5 场战斗（3 普通 + 1 精英 + 1 BOSS）
- 击败 BOSS 后进入“感谢游玩”与“制作人名单（占位）”界面，再回到主菜单
- 主菜单包含“开始游戏 / 退出游戏”
- 药剂系统：初始 1 瓶，战斗胜利后 +1（上限 3），战斗中可点击使用（+10 格挡并抽 1）
- 轻量动画：受击闪白、状态栏受击红闪、手牌悬停上浮、回合提示淡出

## Build (Windows + vcpkg recommended)

1. Install dependencies:
   - CMake
   - Ninja
   - A C++ toolchain (g++, clang++, or MSVC)
2. Configure:
   - `cmake --preset default`
3. Build:
   - `cmake --build --preset default`
4. Run:
   - `./build/MiniSpireSFML.exe`

## Controls

- 主菜单点击按钮：开始 / 退出
- 战斗中点击卡牌：出牌（需满足能量）
- 点击“结束回合”：进入敌方行动
- 点击药剂按钮：消耗 1 瓶药剂（+10 格挡并抽 1 张牌）
- 奖励界面点击三选一卡：加入本局牌组
- 按 `R`：在战斗/奖励/失败阶段重开本局

## Notes

- 文本已全面中文化，并优化了卡牌/日志文本换行展示。
- Windows + MSVC 下默认使用 UTF-8 编译选项，中文显示更稳定。
- Demo 视觉使用联网获取的开源素材（OpenMoji 图标 + Noto CJK 字体）。
- If packaged fonts fail to load, the game tries `C:/Windows/Fonts/arial.ttf`.
- 本局内牌组会成长：奖励卡会进入主牌组并用于后续战斗。
- 战斗临时状态会在每场战斗开始时重置。
- 保存文件 `save_progress.dat` 会记录累计通关次数。
- SFML is fetched and built from source by CMake to avoid local binary ABI mismatches.

## Asset Attribution

- See `assets/THIRD_PARTY_LICENSES.md` for exact sources and licenses.
