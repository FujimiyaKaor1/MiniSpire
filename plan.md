## **Plan: 尖塔风格美术与音频升级**

已完成代码与资源链路调研，并按你确认的方向收敛为可执行方案：手绘暗黑奇幻、仅 CC0/免署名优先、史诗黑暗音频、先交付高品质可落地版。实现上采用“最小代码改动 + 大部分资源替换”，重点是事件双背景与最终 Boss 胜利音乐独立化。

**Steps**

1. 阶段 A（阻塞后续）：建立资源准入规则
2. 仅采纳可验证 CC0 或等价免署名许可资源；排除许可不清、仅预览可用、禁止再分发素材。
3. 锁定统一美术规则：冷灰主调 + 暗金点缀、低饱和高对比、统一光源方向、避免照片感混入。
4. 阶段 B（依赖 A）：背景升级
5. 替换主菜单为尖塔外景、战斗为尖塔内部、商店为路边摊垫子、火堆为篝火场景。
6. 将事件背景拆分为事件1（神殿祭坛）与事件2（森林小路），代码按 currentEventId 渲染对应背景。
7. 阶段 C（可与 B 后半并行）：音频升级
8. 升级场景 BGM（主菜单/战斗/商店/火堆/事件/失败/胜利）为史诗黑暗气质，保留现有命名映射。
9. 升级玩家与敌人攻击/受击音效，先替换 sword\_attack、player\_hit、enemy\_hit，并做音量层级校准。
10. 新增最终 Boss 胜利专属 BGM 映射，仅用于 VictoryThanks；Credits 维持通用胜利曲或单独曲目（二选一）。
11. 阶段 D（依赖 B/C）：合规与回归
12. 更新第三方许可台账，逐条记录文件-来源-许可-作者-下载日期-是否改动。
13. 同步下载脚本或资源说明，保证素材来源可复现。
14. 执行构建、测试、全流程运行与“高级感审查”（色调统一、构图留白、音量一致性）。

**Relevant files**

- [main.cpp](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 背景纹理声明与加载、事件渲染分支、Phase-BGM 映射、SFX 触发点
- [main\_menu.png](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 主菜单背景
- [battle.png](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 战斗背景
- [shop.png](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 商店背景
- [campfire.png](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 火堆背景
- [event.png](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 当前事件背景基线（将拆分为两张）
- [bgm](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 各阶段 BGM 资源目录
- [sfx](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 攻击/受击等音效目录
- [THIRD\_PARTY\_LICENSES.md](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 许可台账
- [download\_beautify\_assets.ps1](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 资源获取流程
- [download\_beautify.js](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 下载映射逻辑
- [generate\_backgrounds.js](vscode-file://vscode-app/c:/Users/csj/AppData/Local/Programs/Microsoft%20VS%20Code/07ff9d6178/resources/app/out/vs/code/electron-browser/workbench/workbench.html) - 背景兜底生成逻辑（需同步事件双背景兜底）

**Verification**

1. 资源完整性检查：背景命名与数量覆盖主菜单、战斗、商店、火堆、事件1、事件2、失败、胜利。
2. 构建验证：cmake --preset default；cmake --build --preset default。
3. 测试验证：ctest --preset default --output-on-failure。
4. 运行验证：主菜单到最终 Boss 通关全流程，确认 BGM/SFX 触发正确且无回退报错。
5. 合规验证：台账中每条来源可回溯到具体文件与链接。
6. 质感验证：统一色调、对比度、空间留白与音量峰值一致性。

**Decisions**

- 视觉：手绘暗黑奇幻（接近尖塔系 roguelike 气质）
- 许可：仅 CC0/免署名优先，安全优先
- 音频：史诗黑暗、强调压迫感
- 交付策略：先完成高品质可落地版，再做二轮精修
- 本轮不包含：玩法数值改动、UI 结构重做、音频设置菜单

我已将同版完整计划同步到会话计划文件：memories/session/plan.md。如果你确认这份计划，我下一步会按这个顺序进入实现执行。

已完成本轮要求：完成只读调研、联网许可核验、关键需求对齐，并已将可执行升级方案写入会话计划文件，当前计划可直接用于后续实施。

已按要求补齐完成标记流程：我已先发送简要完成说明，并调用了 task\_complete 工具完成本轮任务收尾。
