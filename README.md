# MiniSpireSFML

一个基于 C++ + SFML 的类《杀戮尖塔》最小可玩 Demo。

当前阶段：可演示版本（已接入联网开源素材并中文化）。

Implemented scope:
- 1 名可玩角色（铁甲战士风格）
- 38 张战士卡牌（含 18 张新增卡与全卡升级版本）
- 4 种普通敌人
- 3 种精英敌人
- 1 个 BOSS
- 固定 15 层爬塔流程（战斗/营火/商店/事件）
- 击败 BOSS 后进入“感谢游玩”与“制作人名单（占位）”界面，再回到主菜单
- 主菜单包含“开始游戏 / 退出游戏”
- 金币系统：初始 99 金币，可在商店购买卡牌、提升药剂上限、移除卡牌
- 药剂系统：初始 1 瓶，战斗胜利后 +1（受上限约束），战斗中可点击使用（+10 格挡并抽 1）
- 遗物系统：5 个遗物，精英战胜利后掉落且不重复（首回合能量、战前回血、反伤等效果）
- 轻量动画：受击闪白、状态栏受击红闪、手牌悬停上浮、出牌短过渡动画、回合提示淡出、战斗浮动文字反馈
- 轻量 UI 主题美化：统一了顶部状态区、敌人面板、按钮描边与背景渐变配色

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
5. Smoke self-test:
   - `ctest --preset default --output-on-failure`

## Controls

- 主菜单点击按钮：开始 / 退出
- 战斗中点击卡牌：出牌（需满足能量）
- 点击“结束回合”：进入敌方行动
- 点击药剂按钮：消耗 1 瓶药剂（+10 格挡并抽 1 张牌）
- 点击右上角状态区：查看已拥有卡牌；点击“查看遗物总览”：查看已拥有遗物
- 奖励界面点击三选一卡：加入本局牌组
- 营火界面：可选择“休息（回复 30 生命）”或“锻造（升级 1 张卡）”
- 商店界面：可购买随机卡牌、提升药剂上限、购买移除服务
- 事件界面：按事件文本选择左右选项执行分支结果
- 商店与营火增加操作提示（金币不足、已售出、剩余选择数等）
- 按 `R`：在战斗/奖励/失败阶段重开本局
- 按 `T`：开关战斗动画（便于低性能设备）

## Notes

- 文本已全面中文化，并优化了卡牌/日志文本换行展示。
- 敌我增益/减益与伤害会显示浮动文字，战斗读数更清晰。
- 敌方回合加入前摇/后摇节奏，意图到出手的可读性更强。
- 首次运行会自动生成 `game_config.ini`，可配置帧率、动画开关、浮动文字开关和文本缩放。
- 当贴图缺失时会绘制“资源缺失”占位块，便于快速定位资源问题。
- Windows + MSVC 下默认使用 UTF-8 编译选项，中文显示更稳定。
- Demo 视觉使用联网获取的开源素材（OpenMoji 图标 + Noto CJK 字体）。
- If packaged fonts fail to load, the game tries `C:/Windows/Fonts/arial.ttf`.
- 本局内牌组会成长：奖励卡会进入主牌组并用于后续战斗。
- 起始牌组已调整为 10 张（4 打击、4 防御、1 重击、1 快速检索）。
- 平衡微调：血瓶为每战回复 5 生命；荆棘徽记反伤为 1；精英战遗物掉落为概率触发（非必掉）。
- 营火锻造会将卡牌升级为 `卡名+`，升级后优先提升数值，部分高费卡会降费。
- 战斗日志已缩小字号并移动到左上侧，玩家属性条移动到左下，敌人意图增加高亮框。
- 已进行多轮平衡迭代：6-12层中段曲线更平滑，12-15层后段压迫感与奖励同步上调。
- 第5轮微调：商店对抽牌+能量类引擎牌提高溢价，事件金币暴利分支由100下调到80，降低后段滚雪球。
- 新增手动回归清单：`REGRESSION_CHECKLIST.md`，用于验证放血/盛怒/薪火之源/添柴及其升级版行为。
- 纹理缺失时会显示“资源缺失”占位块，避免静默空白。
- 战斗临时状态会在每场战斗开始时重置。
- 保存文件 `save_progress.dat` 会记录累计通关次数。
- SFML is fetched and built from source by CMake to avoid local binary ABI mismatches.

## Safe Online Asset Update

- 本仓库提供了安全可审计的敌人素材下载脚本：
- `scripts/download_enemy_assets_openmoji.ps1`
- 脚本仅从 OpenMoji 官方 GitHub raw 地址下载 5 张敌人 PNG 到 `assets/images/enemies/`。
- 本仓库提供了安全可审计的 UI emoji 下载脚本：
- `scripts/download_ui_emoji_openmoji.ps1`
- 脚本仅从 OpenMoji 官方 GitHub raw 地址下载 4 张状态区 emoji PNG 到 `assets/images/ui/`。
- 下载后请同步核对 `assets/THIRD_PARTY_LICENSES.md` 中的来源记录。

## Release Notes

### v0.2.0-demo
- 新增战斗浮动文字反馈（伤害、增益、减益、药剂）。
- 新增出牌短过渡动画与敌方回合前摇/后摇节奏。
- 新增战斗动画热切换（`T` 键）与配置文件读取（`game_config.ini`）。
- 新增贴图缺失占位绘制与资源加载告警日志。
- 调整前两战数值曲线与初始牌组，增加奖励低费保底。

## Demo 推荐参数

- `max_fps=60`
- `enable_animations=true`
- `enable_floating_text=true`
- `text_scale=1.0`

## Known Limitations

- 当前项目仍为单文件核心逻辑，后续会继续模块化拆分。
- 当前仅提供最小化自动回归（`smoke_self_test`），覆盖配置解析与阶段跳转合法性。
- `smoke_self_test` 已扩展覆盖楼层脚本顺序、商店定价边界与价值单调性、战斗金币曲线、事件选项与金币奖励边界校验。
- `smoke_self_test` 已覆盖起始牌组 10 张构成、遗物唯一性与关键遗物触发（首回合能量、战前回血、反伤数值）校验。
- 目前地图是固定 15 层脚本，尚未实现分支路线与随机房间池。

## Asset Attribution

- See `assets/THIRD_PARTY_LICENSES.md` for exact sources and licenses.
