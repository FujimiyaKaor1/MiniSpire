## 用户需求

将游戏整体美化，包括以下内容：

### 背景图片（7种场景）
- 主菜单背景图片
- 战斗背景图片
- 火堆背景图片
- 商店背景图片
- 事件背景图片
- 战败背景图片
- 感谢游玩背景图片

### 敌人图片（8种不同敌人）
- 普通敌人：黏液斗士、邪教徒、寄生突袭者、刀盾卫兵
- 精英敌人：装甲骑士、鲜血斗士、诅咒祭司
- BOSS：尖塔守卫

### 卡牌美化
- 卡牌字体美化（使用更适合游戏的中文字体）
- 升级后的卡牌字体颜色变为绿色

### UI图标
- 为玩家生命值添加图标
- 为玩家能量/费用添加图标

### 音频系统
- 添加背景音乐
- 添加攻击音效
- 添加受伤音效

### 约束条件
- 文本位置不需要变动
- 保证安全的情况下联网获取开源素材
- 素材来源需符合CC0或免费商用许可

## 技术栈

- **现有技术**：C++17 + SFML 2.6.1 + CMake
- **音频系统**：SFML内置音频模块
  - `sf::Music`：用于背景音乐（支持流式播放）
  - `sf::SoundBuffer` + `sf::Sound`：用于短音效

## 素材来源

### 图片素材（CC0许可）
- **Kenney.nl**：提供UI、地牢、RPG等游戏素材，CC0许可可商用
- **OpenGameArt.org**：Instant Dungeon Art Pack等素材包

### 音频素材
- **Mixkit.co**：免费可商用音效（剑击、受伤等）
- **Pixabay.com/music**：免费背景音乐

### 字体素材
- **站酷酷黑体**：免费商用中文字体，适合游戏UI
- **站酷庆科黄油体**：适合标题展示

## 实现方案

### 1. 背景图片系统
在Game类中添加背景纹理变量，修改render函数在各阶段绘制背景：
```cpp
sf::Texture bgMainMenuTex;
sf::Texture bgBattleTex;
sf::Texture bgCampfireTex;
sf::Texture bgShopTex;
sf::Texture bgEventTex;
sf::Texture bgDefeatTex;
sf::Texture bgVictoryTex;
```

### 2. 敌人图片系统
扩展enemyPortrait()函数，为8种敌人分配不同纹理：
```cpp
sf::Texture enemySlimeTex;      // 黏液斗士
sf::Texture enemyCultistTex;    // 邪教徒
sf::Texture enemyParasiteTex;   // 寄生突袭者
sf::Texture enemyGuardTex;      // 刀盾卫兵
sf::Texture eliteKnightTex;     // 装甲骑士
sf::Texture eliteFighterTex;    // 鲜血斗士
sf::Texture elitePriestTex;     // 诅咒祭司
sf::Texture bossTex;            // 尖塔守卫
```

### 3. 升级卡牌绿色显示
修改cardColor()函数，增加升级卡牌判断：
```cpp
sf::Color cardColor(CardType type, bool isUpgraded) const {
    if (isUpgraded) return sf::Color(80, 200, 120); // 绿色
    // 原有颜色逻辑...
}
```

### 4. 音频系统架构
```cpp
// 背景音乐
sf::Music bgmMainMenu;
sf::Music bgmBattle;
sf::Music bgmCampfire;
sf::Music bgmShop;

// 音效缓冲
sf::SoundBuffer sfxAttackBuffer;
sf::SoundBuffer sfxHitBuffer;
sf::SoundBuffer sfxHurtBuffer;
sf::Sound sfxAttack;
sf::Sound sfxHit;
sf::Sound sfxHurt;
```

### 5. 目录结构调整
```
assets/
├── images/
│   ├── backgrounds/     # [NEW] 背景图片
│   ├── enemies/         # [MODIFY] 8种敌人图片
│   ├── cards/           # 卡牌图标
│   └── ui/              # UI图标
├── audio/               # [NEW] 音频文件
│   ├── music/           # 背景音乐
│   └── sfx/             # 音效
└── fonts/               # 字体文件
```

## 实现注意事项

1. **素材下载脚本**：创建PowerShell脚本安全下载素材，记录来源和许可
2. **音频延迟**：音效预加载到缓冲区，避免播放延迟
3. **内存管理**：背景图片尺寸控制在合理范围（1280x720）
4. **向后兼容**：素材加载失败时使用占位图，不影响游戏运行
5. **音量控制**：可考虑添加配置项控制音量

## Agent Extensions

### Skill
- **browser-automation**：用于自动访问素材网站（Kenney.nl、Mixkit.co、Pixabay等），批量下载符合游戏风格的免费可商用素材
  - 目的：安全获取背景图片、音效、音乐素材
  - 预期结果：下载的素材保存到assets目录对应位置

### SubAgent
- **code-explorer**：用于深入探索代码库，确保所有需要修改的渲染函数都被识别
  - 目的：找到所有需要添加背景绘制、音效触发的代码位置
  - 预期结果：完整的修改点清单
