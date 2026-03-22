/**
 * MiniSpireSFML - 一款类《杀戮尖塔》的卡牌 roguelike 游戏
 * 
 * 项目概述：
 * - 单文件架构，所有游戏逻辑集中在 main.cpp 中
 * - 使用 SFML 库进行图形渲染和音频播放
 * - 支持跨平台编译（Windows/Linux/macOS）
 * 
 * 核心系统：
 * - 卡牌战斗系统：支持攻击、技能、能力三种卡牌类型
 * - 敌人AI系统：基于意图模式的敌人行为
 * - 遗物系统：提供被动增益效果
 * - 事件系统：随机事件增加游戏变数
 * - 商店系统：购买卡牌、遗物、药水
 * 
 * 作者：杨立鑫、宋金林
 */

#include <SFML/Graphics.hpp>

// ============================================================================
// 音频系统配置
// SFML Audio 是可选模块，不可用时优雅降级
// ============================================================================
#ifndef USE_AUDIO
#define USE_AUDIO 0
#endif

#if USE_AUDIO
#include <SFML/Audio.hpp>
#endif

#if USE_AUDIO
using SfxBuffer = sf::SoundBuffer;
#else
struct SfxBuffer {};  // 空结构体占位，编译期兼容
#endif

// ============================================================================
// 标准库头文件
// ============================================================================
#include <algorithm>      // std::find, std::remove_if 等算法
#include <cctype>         // 字符处理
#include <cmath>          // 数学函数
#include <ctime>          // 时间函数
#include <cstdint>        // 固定宽度整数类型
#include <deque>          // 双端队列（用于战斗日志）
#include <fstream>        // 文件流（配置读取）
#include <iostream>       // 标准输入输出
#include <filesystem>     // 文件系统操作
#include <random>         // 随机数生成器
#include <sstream>        // 字符串流
#include <stdexcept>      // 异常类
#include <string>         // 字符串
#include <unordered_map>  // 哈希映射（卡牌查找）
#include <unordered_set>  // 哈希集合
#include <vector>         // 动态数组

// ============================================================================
// 枚举类型定义
// ============================================================================

/**
 * @brief 卡牌类型枚举
 * - Attack: 攻击牌，造成伤害
 * - Skill: 技能牌，提供格挡或效果
 * - Power: 能力牌，持续生效的增益
 */
enum class CardType { Attack, Skill, Power };

/**
 * @brief 游戏阶段枚举
 * 控制游戏状态机，决定当前渲染和交互逻辑
 */
enum class Phase { 
    MainMenu,       // 主菜单
    Battle,         // 战斗场景
    Reward,         // 奖励选择
    Campfire,       // 营火休息
    Shop,           // 商店
    Event,          // 随机事件
    DeckEdit,       // 牌组编辑（升级/删除）
    VictoryThanks,  // 胜利感谢页
    Credits,        // 制作人名单
    Defeat,         // 战败页面
    Rules           // 玩法介绍
};

/**
 * @brief 房间类型枚举
 * 用于地图生成和场景切换
 */
enum class RoomType { 
    Battle,     // 战斗房间
    Campfire,   // 营火房间
    Shop,       // 商店房间
    Event       // 事件房间
};

/**
 * @brief 牌组编辑模式
 * 控制牌组编辑界面的行为
 */
enum class DeckEditMode { 
    None,       // 无编辑
    UpgradeOne, // 升级一张卡
    RemoveOne,  // 删除一张卡
    RemoveTwo   // 删除两张卡
};

/**
 * @brief 牌堆覆盖层类型
 * 用于显示不同牌堆的详细内容
 */
enum class PileOverlayType { 
    None,       // 无覆盖层
    Draw,       // 抽牌堆
    Discard,    // 弃牌堆
    Exhaust,    // 消耗堆
    Deck        // 完整牌组
};

/**
 * @brief 遗物类型枚举
 * 每种遗物提供独特的被动效果
 */
enum class RelicType { 
    EmberCharm,      // 余烬护符：每回合开始获得1能量
    IronSigil,       // 铁纹章：战斗开始获得4格挡
    BloodVial,       // 血瓶：战斗开始治疗2点生命
    TacticianLens,   // 战术透镜：首回合多抽2张
    ThornEmblem      // 荆棘徽记：受击反伤3点
};

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 卡牌定义结构体
 * 存储一张卡牌的所有属性信息
 * 
 * @note 卡牌ID规则：
 * - 1-44: 基础卡牌
 * - 45-50: 新增卡牌
 * - ID+100: 升级版卡牌（如ID=1的升级版为101）
 */
struct CardDef {
    int id;                     // 卡牌唯一标识符
    std::string name;           // 卡牌名称（中文）
    CardType type;              // 卡牌类型（攻击/技能/能力）
    int cost;                   // 能量消耗
    int damage;                 // 基础伤害值
    int hits;                   // 攻击次数（用于多段攻击）
    int block;                  // 提供的格挡值
    int gainStrength;           // 获得的力量值
    int applyVulnerable;        // 施加的易伤层数
    int applyWeak;              // 施加的虚弱层数
    int draw;                   // 抽牌数量
    int gainEnergy;             // 获得的能量
    int loseHp;                 // 失去的生命值（代价）
    bool exhaust;               // 是否消耗（打出后移出本局游戏）
    bool grantsStrengthPerTurn; // 是否每回合获得力量
    std::string desc;           // 卡牌描述文本
    
    // 以下为扩展字段，有默认值
    bool healFromDamage = false;        // 是否从伤害中治疗（吸血效果）
    bool grantsEnergyPerTurn = false;   // 是否每回合获得能量
    bool redrawByExhaustingHand = false;// 消耗手牌后是否重抽
    bool eventOnly = false;             // 是否仅限事件获取（如邪恶之刃）
};

/**
 * @brief 地图楼层节点
 * 表示地图上的一个房间
 */
struct FloorNode {
    RoomType type = RoomType::Battle;   // 房间类型
    int enemyIndex = -1;                 // 敌人索引（-1表示随机）
    int eventId = 0;                     // 事件ID
};

/**
 * @brief 敌人意图结构体
 * 定义敌人回合的行动模式
 * 
 * @note 意图系统参考《杀戮尖塔》：
 * - 敌人意图在玩家回合结束时确定
 * - 玩家可以根据意图制定策略
 */
struct Intent {
    std::string label;      // 意图标签（用于UI显示）
    int damage;             // 伤害值
    int hits;               // 攻击次数
    int block;              // 获得的格挡
    int gainStrength;       // 获得的力量
    int applyWeak;          // 施加的虚弱
    int applyVulnerable;    // 施加的易伤
};

/**
 * @brief 敌人定义结构体
 * 存储敌人的基础属性和行为模式
 */
struct EnemyDef {
    std::string name;           // 敌人名称
    int maxHp;                  // 最大生命值
    bool elite;                 // 是否为精英敌人
    bool boss;                  // 是否为Boss
    std::vector<Intent> pattern;// 意图模式循环
};

/**
 * @brief 战斗单位状态
 * 玩家和敌人共用的状态结构
 */
struct Combatant {
    int hp = 0;             // 当前生命值
    int maxHp = 0;          // 最大生命值
    int block = 0;          // 当前格挡值
    int strength = 0;       // 力量（增加伤害）
    int weak = 0;           // 虚弱层数（减少伤害）
    int vulnerable = 0;     // 易伤层数（受到更多伤害）
};

/**
 * @brief 浮动文字效果
 * 用于显示伤害/治疗等数字动画
 */
struct FloatingText {
    sf::String text;        // 显示文本
    sf::Vector2f position;  // 屏幕位置
    sf::Vector2f velocity;  // 移动速度（向上飘动）
    sf::Color color;        // 文字颜色
    float remaining = 0.0f; // 剩余显示时间
    float duration = 0.0f;  // 总显示时长
};

/**
 * @brief 待处理的卡牌打出动画
 * 用于卡牌打出时的动画效果
 */
struct PendingCardPlay {
    bool active = false;        // 是否正在播放动画
    int handIndex = -1;         // 手牌索引
    int cardId = -1;            // 卡牌ID
    float elapsed = 0.0f;       // 已播放时间
    float duration = 0.16f;     // 动画总时长（秒）
    sf::FloatRect fromRect;     // 起始位置
    sf::Vector2f target;        // 目标位置
};

/**
 * @brief 待处理的敌人回合动画
 * 控制敌人攻击的动画时序
 */
struct PendingEnemyTurn {
    bool active = false;        // 是否正在播放动画
    bool resolved = false;      // 是否已执行伤害
    float elapsed = 0.0f;       // 已播放时间
    float windup = 0.22f;       // 前摇时间（攻击准备）
    float recover = 0.12f;      // 后摇时间（攻击恢复）
};

/**
 * @brief 应用配置结构体
 * 从配置文件读取的运行时设置
 */
struct AppConfig {
    unsigned int maxFps = 60;           // 最大帧率
    bool enableAnimations = true;       // 是否启用动画
    bool enableFloatingText = true;     // 是否显示浮动文字
    float textScale = 1.0f;             // 文字缩放比例
};

// ============================================================================
// 游戏平衡性常量
// 集中管理数值，便于调整游戏难度
// ============================================================================
namespace Balance {
constexpr int kBaseTurnEnergy = 3;             // 每回合基础能量
constexpr int kStartingMaxHp = 80;             // 初始最大生命值
constexpr int kStartingGold = 99;              // 初始金币
constexpr int kStartingPotionCount = 1;        // 初始药水数量
constexpr int kStartingPotionMax = 4;          // 药水上限
constexpr int kStartingPotionCapacityPrice = 65;   // 购买药水价格
constexpr int kStartingDeckRemovePrice = 60;       // 删除卡牌价格
constexpr int kEliteRelicDropChance = 70;          // 精英掉落遗物概率(%)
constexpr int kBloodVialHeal = 5;                  // 血瓶治疗量
} // namespace Balance

/**
 * @brief 游戏主类
 * 
 * 职责：
 * - 管理游戏状态机（Phase）
 * - 处理用户输入
 * - 渲染游戏画面
 * - 管理战斗、商店、事件等子系统
 * 
 * 设计模式：
 * - 单例模式（隐式）：整个游戏只有一个Game实例
 * - 状态模式：通过Phase枚举切换不同游戏阶段
 * 
 * @note 所有游戏逻辑都在此类中实现，采用单文件架构
 */
class Game {
public:
    /**
     * @brief 生成默认地图布局
     * @return 15层楼的房间配置
     * 
     * 地图结构：
     * - 第1-2层：普通战斗（敌人0-1）
     * - 第3层：营火
     * - 第4-5层：战斗+精英（敌人2、4）
     * - 第6层：商店
     * - 第7层：战斗（敌人3）
     * - 第8层：事件1
     * - 第9层：营火
     * - 第10层：精英战斗（敌人5）
     * - 第11层：商店
     * - 第12层：精英战斗（敌人6）
     * - 第13层：事件2
     * - 第14层：营火
     * - 第15层：Boss战（敌人7）
     */
    static std::vector<FloorNode> defaultFloorPlan() {
        return {
            {RoomType::Battle, 0, 0},      // 第1层：普通敌人
            {RoomType::Battle, 1, 0},      // 第2层：普通敌人
            {RoomType::Campfire, -1, 0},   // 第3层：营火休息
            {RoomType::Battle, 2, 0},      // 第4层：普通敌人
            {RoomType::Battle, 4, 0},      // 第5层：精英敌人
            {RoomType::Shop, -1, 0},       // 第6层：商店
            {RoomType::Battle, 3, 0},      // 第7层：普通敌人
            {RoomType::Event, -1, 1},      // 第8层：事件1
            {RoomType::Campfire, -1, 0},   // 第9层：营火休息
            {RoomType::Battle, 5, 0},      // 第10层：精英敌人
            {RoomType::Shop, -1, 0},       // 第11层：商店
            {RoomType::Battle, 6, 0},      // 第12层：精英敌人
            {RoomType::Event, -1, 2},      // 第13层：事件2
            {RoomType::Campfire, -1, 0},   // 第14层：营火休息
            {RoomType::Battle, 7, 0}       // 第15层：Boss战
        };
    }

    static bool runSelfTest(std::ostream& out) {
        bool success = true;
        auto expect = [&](bool condition, const char* name) {
            out << (condition ? "[OK] " : "[FAIL] ") << name << '\n';
            success = success && condition;
        };

        expect(parseBool("true"), "parseBool true");
        expect(parseBool("YES"), "parseBool yes");
        expect(!parseBool("off"), "parseBool off");

        const AppConfig parsed = parseConfigLines(
            {
                "max_fps=999",
                "enable_animations=off",
                "enable_floating_text=yes",
                "text_scale=0.2",
                "text_scale=1.4"
            },
            nullptr);

        expect(parsed.maxFps == 240u, "max_fps clamp");
        expect(!parsed.enableAnimations, "enable_animations parse");
        expect(parsed.enableFloatingText, "enable_floating_text parse");
        expect(std::fabs(parsed.textScale - 1.4f) < 0.0001f, "text_scale parse");

        expect(isValidTransition(Phase::MainMenu, Phase::Battle), "phase transition main menu -> battle");
        expect(isValidTransition(Phase::Campfire, Phase::DeckEdit), "phase transition campfire -> deck edit");
        expect(isValidTransition(Phase::Shop, Phase::Battle), "phase transition shop -> battle");
        expect(!isValidTransition(Phase::Reward, Phase::Credits), "phase transition reward -> credits invalid");
        expect(!isValidTransition(Phase::Event, Phase::Defeat), "phase transition event -> defeat invalid");

        const std::vector<FloorNode> plan = defaultFloorPlan();
        expect(plan.size() == 15u, "floor plan has 15 floors");
        expect(plan[0].type == RoomType::Battle && plan[0].enemyIndex == 0, "floor 1 battle enemy 1");
        expect(plan[2].type == RoomType::Campfire, "floor 3 campfire");
        expect(plan[5].type == RoomType::Shop, "floor 6 shop");
        expect(plan[7].type == RoomType::Event && plan[7].eventId == 1, "floor 8 event 1");
        expect(plan[12].type == RoomType::Event && plan[12].eventId == 2, "floor 13 event 2");
        expect(plan[14].type == RoomType::Battle && plan[14].enemyIndex == 7, "floor 15 final boss");

        expect(parseWinsFromProgressLine("wins=12") == 12, "progress parse valid wins");
        expect(parseWinsFromProgressLine("wins=-7") == 0, "progress parse clamps negative wins");
        expect(parseWinsFromProgressLine("wins=abc") == 0, "progress parse invalid numeric");
        expect(parseWinsFromProgressLine("foo=9") == 0, "progress parse ignores unknown key");
        expect(extractWinsFromProgressLines({"wins=3", "wins=7"}) == 7, "progress parse last wins entry");
        expect(extractWinsFromProgressLines({"#comment", "foo=2"}) == 0, "progress parse default wins");
        expect(serializeWinsProgressLine(-9) == "wins=0", "progress serialize clamps negative");
        expect(serializeWinsProgressLine(15) == "wins=15", "progress serialize positive value");
        expect(isTextureSizeAvailable(sf::Vector2u(0u, 0u)) == false, "texture fallback for empty size");
        expect(isTextureSizeAvailable(sf::Vector2u(1u, 0u)) == false, "texture fallback for zero height");
        expect(isTextureSizeAvailable(sf::Vector2u(0u, 2u)) == false, "texture fallback for zero width");
        expect(isTextureSizeAvailable(sf::Vector2u(8u, 8u)) == true, "texture available for positive size");

        std::ostringstream validationSink;
        CardDef validCard{};
        validCard.id = 1;
        validCard.cost = 1;
        validCard.hits = 1;
        expect(validateCardDefBasic(validCard, validationSink), "card validation accepts valid card");
        CardDef invalidCostCard = validCard;
        invalidCostCard.cost = -1;
        expect(!validateCardDefBasic(invalidCostCard, validationSink), "card validation rejects invalid cost");
        CardDef invalidHitsCard = validCard;
        invalidHitsCard.hits = 0;
        expect(!validateCardDefBasic(invalidHitsCard, validationSink), "card validation rejects invalid hits");

        EnemyDef validEnemy{"valid enemy", 10, false, false, {{"攻击 1", 1, 1, 0, 0, 0, 0}}};
        expect(validateEnemyDefBasic(validEnemy, 0, validationSink), "enemy validation accepts valid enemy");
        EnemyDef invalidEnemyHp = validEnemy;
        invalidEnemyHp.maxHp = 0;
        expect(!validateEnemyDefBasic(invalidEnemyHp, 0, validationSink), "enemy validation rejects hp <= 0");
        EnemyDef invalidEnemyPattern = validEnemy;
        invalidEnemyPattern.pattern.clear();
        expect(!validateEnemyDefBasic(invalidEnemyPattern, 0, validationSink), "enemy validation rejects empty pattern");

        FloorNode validBattleNode{RoomType::Battle, 1, 0};
        expect(validateFloorNodeBasic(validBattleNode, 0, 8, validationSink), "floor validation accepts valid battle node");
        FloorNode invalidBattleNode{RoomType::Battle, 99, 0};
        expect(!validateFloorNodeBasic(invalidBattleNode, 0, 8, validationSink), "floor validation rejects out-of-range enemy index");
        FloorNode invalidEventNode{RoomType::Event, -1, 9};
        expect(!validateFloorNodeBasic(invalidEventNode, 0, 8, validationSink), "floor validation rejects invalid event id");

        CardDef cheap{};
        cheap.cost = 0;
        cheap.damage = 0;
        cheap.block = 0;
        cheap.draw = 0;
        cheap.grantsStrengthPerTurn = false;
        cheap.exhaust = false;
        expect(calculateCardPrice(cheap) == 40, "shop price baseline card");

        CardDef expensive{};
        expensive.cost = 3;
        expensive.damage = 20;
        expensive.block = 0;
        expensive.draw = 0;
        expensive.gainEnergy = 0;
        expensive.grantsStrengthPerTurn = true;
        expensive.exhaust = false;
        expect(calculateCardPrice(expensive) == 146, "shop price high value card");

        CardDef drawEngine{};
        drawEngine.cost = 1;
        drawEngine.damage = 0;
        drawEngine.block = 0;
        drawEngine.draw = 4;
        drawEngine.gainEnergy = 1;
        drawEngine.grantsStrengthPerTurn = false;
        drawEngine.exhaust = true;
        expect(calculateCardPrice(drawEngine) == 106, "shop price draw engine card");

        CardDef perTurnEnergy{};
        perTurnEnergy.cost = 2;
        perTurnEnergy.damage = 0;
        perTurnEnergy.block = 0;
        perTurnEnergy.draw = 0;
        perTurnEnergy.gainEnergy = 0;
        perTurnEnergy.grantsStrengthPerTurn = false;
        perTurnEnergy.grantsEnergyPerTurn = true;
        perTurnEnergy.redrawByExhaustingHand = false;
        perTurnEnergy.exhaust = true;
        expect(calculateCardPrice(perTurnEnergy) == 102, "shop price per-turn energy card");

        CardDef redrawTool{};
        redrawTool.cost = 1;
        redrawTool.damage = 0;
        redrawTool.block = 0;
        redrawTool.draw = 0;
        redrawTool.gainEnergy = 0;
        redrawTool.grantsStrengthPerTurn = false;
        redrawTool.grantsEnergyPerTurn = false;
        redrawTool.redrawByExhaustingHand = true;
        redrawTool.exhaust = false;
        expect(calculateCardPrice(redrawTool) == 70, "shop price redraw card");

        CardDef midValue{};
        midValue.cost = 1;
        midValue.damage = 8;
        midValue.block = 0;
        midValue.draw = 2;
        midValue.gainEnergy = 0;
        midValue.grantsStrengthPerTurn = false;
        midValue.exhaust = false;

        CardDef highValue = midValue;
        highValue.draw = 4;
        highValue.gainEnergy = 1;
        highValue.exhaust = true;
        expect(calculateCardPrice(highValue) > calculateCardPrice(midValue), "shop price monotonic high value");

        CardDef bounded{};
        bounded.cost = 10;
        bounded.damage = 40;
        bounded.block = 0;
        bounded.draw = 0;
        bounded.grantsStrengthPerTurn = true;
        bounded.exhaust = false;
        expect(calculateCardPrice(bounded) == 190, "shop price upper clamp");

        expect(battleGoldForFloor(0) == 35, "battle gold floor 1");
        expect(battleGoldForFloor(5) == 50, "battle gold floor 6");
        expect(battleGoldForFloor(14) == 85, "battle gold floor 15 cap");
        expect(battleRoomBonus(true, false) == 12, "battle gold elite bonus");
        expect(battleRoomBonus(false, true) == 25, "battle gold boss bonus");

        expect(isValidEventChoice(1, 0), "event 1 option 0 valid");
        expect(isValidEventChoice(1, 1), "event 1 option 1 valid");
        expect(isValidEventChoice(2, 0), "event 2 option 0 valid");
        expect(isValidEventChoice(2, 1), "event 2 option 1 valid");
        expect(!isValidEventChoice(1, 2), "event invalid option rejected");
        expect(!isValidEventChoice(99, 0), "unknown event rejected");
        expect(eventGoldReward(1, 1) == 80, "event 1 gold reward amount");
        expect(eventGoldReward(1, 0) == 0, "event 1 hp branch gold reward");
        expect(eventGoldReward(2, 1) == 0, "event 2 no gold reward");
        expect(turnStartEnergy(0) == 3, "turn start base energy");
        expect(turnStartEnergy(2) == 5, "turn start bonus energy");
        expect(validateFlatEnergyCurve(2, 3, 5), "stacked energy flat curve");
        expect(stokeRedrawCountFromHandSize(1) == 0, "stoke redraw count single card");
        expect(stokeRedrawCountFromHandSize(4) == 3, "stoke redraw count hand size 4");
        expect(simulateDrawCount(4, 0, 6, 0) == 4, "stoke draw with reshuffle");
        expect(simulateDrawCount(9, 0, 8, 0) == 8, "stoke draw bounded by total cards");
        expect(simulateDrawCount(5, 20, 0, 8) == 2, "stoke draw respects hand limit");

        std::vector<int> testHand{11, 12, 13};
        std::vector<int> testExhaust;
        moveAllHandCardsToExhaust(testHand, testExhaust);
        expect(testHand.empty(), "stoke clears remaining hand");
        expect(testExhaust.size() == 3u, "stoke moves all remaining cards to exhaust");

        CardDef bloodletting{};
        bloodletting.id = 37;
        bloodletting.name = "放血";
        bloodletting.type = CardType::Skill;
        bloodletting.cost = 0;
        bloodletting.damage = 0;
        bloodletting.hits = 1;
        bloodletting.block = 0;
        bloodletting.gainStrength = 0;
        bloodletting.applyVulnerable = 0;
        bloodletting.applyWeak = 0;
        bloodletting.draw = 0;
        bloodletting.gainEnergy = 2;
        bloodletting.loseHp = 3;
        bloodletting.exhaust = false;
        bloodletting.grantsStrengthPerTurn = false;
        bloodletting.desc = "失去 3 点生命，获得 2 点能量";
        const CardDef bloodlettingUp = buildUpgradedCard(bloodletting);
        expect(bloodlettingUp.cost == 0 && bloodlettingUp.gainEnergy == 3 && bloodlettingUp.loseHp == 3, "upgrade bloodletting exact");
        expect(netEnergyAfterPlay(3, bloodletting.cost, bloodletting.gainEnergy) == 5, "bloodletting net energy");

        CardDef fury{};
        fury.id = 38;
        fury.name = "盛怒";
        fury.type = CardType::Skill;
        fury.cost = 1;
        fury.damage = 0;
        fury.hits = 1;
        fury.block = 0;
        fury.gainStrength = 0;
        fury.applyVulnerable = 0;
        fury.applyWeak = 0;
        fury.draw = 0;
        fury.gainEnergy = 2;
        fury.loseHp = 0;
        fury.exhaust = true;
        fury.grantsStrengthPerTurn = false;
        fury.desc = "获得 2 点能量，消耗";
        const CardDef furyUp = buildUpgradedCard(fury);
        expect(furyUp.cost == 0 && furyUp.gainEnergy == 2, "upgrade fury exact");
        expect(calculateCardPrice(fury) == 76, "shop price fury burst energy");
        expect(netEnergyAfterPlay(3, fury.cost, fury.gainEnergy) == 4, "fury net energy");
        expect(netEnergyAfterPlay(3, furyUp.cost, furyUp.gainEnergy) == 5, "fury+ net energy");
        expect(equalsIntVector(simulateEnergyAfterPlaySequence(3, {{bloodletting.cost, bloodletting.gainEnergy}, {fury.cost, fury.gainEnergy}, {furyUp.cost, furyUp.gainEnergy}}), {5, 6, 8}), "energy combo sequence bloodletting->fury->fury+");

        CardDef source{};
        source.id = 39;
        source.name = "薪火之源";
        source.type = CardType::Power;
        source.cost = 2;
        source.damage = 0;
        source.hits = 1;
        source.block = 0;
        source.gainStrength = 0;
        source.applyVulnerable = 0;
        source.applyWeak = 0;
        source.draw = 0;
        source.gainEnergy = 0;
        source.loseHp = 0;
        source.exhaust = true;
        source.grantsStrengthPerTurn = false;
        source.grantsEnergyPerTurn = true;
        source.desc = "每回合开始时获得 1 点能量，消耗";
        const CardDef sourceUp = buildUpgradedCard(source);
        expect(sourceUp.cost == 1 && sourceUp.grantsEnergyPerTurn, "upgrade source of flame exact");
        expect(calculateCardPrice(sourceUp) == 92, "shop price source+ low cost per-turn energy");
        expect(equalsIntVector(simulateTurnStartEnergyWithSourcePlays({1, 0, 0}), {3, 4, 4}), "source cross-turn energy single stack");
        expect(equalsIntVector(simulateTurnStartEnergyWithSourcePlays({1, 1, 0}), {3, 4, 5}), "source cross-turn energy double stack");

        CardDef stoke{};
        stoke.id = 40;
        stoke.name = "添柴";
        stoke.type = CardType::Skill;
        stoke.cost = 1;
        stoke.damage = 0;
        stoke.hits = 1;
        stoke.block = 0;
        stoke.gainStrength = 0;
        stoke.applyVulnerable = 0;
        stoke.applyWeak = 0;
        stoke.draw = 0;
        stoke.gainEnergy = 0;
        stoke.loseHp = 0;
        stoke.exhaust = false;
        stoke.grantsStrengthPerTurn = false;
        stoke.redrawByExhaustingHand = true;
        stoke.desc = "消耗手牌中其余所有卡牌，然后抽取等量卡牌";
        const CardDef stokeUp = buildUpgradedCard(stoke);
        expect(stokeUp.cost == 0 && stokeUp.redrawByExhaustingHand, "upgrade stoke exact");
        expect(simulateStokeDrawAfterExhaust(5, 6, 0) == 4, "stoke full redraw when draw pile sufficient");
        expect(simulateStokeDrawAfterExhaust(5, 2, 1) == 3, "stoke redraw limited by available piles");
        expect(simulateStokeDrawAfterExhaust(10, 20, 0) == 9, "stoke redraw with large hand");
        expect(equalsIntVector(simulateStokeConsecutiveTurnDraws({5, 4}, 8, 3), {4, 3}), "stoke consecutive turn draws");
        const std::vector<int> starterDeck = starterDeckTemplate();
        expect(starterDeck.size() == 10u, "starter deck has 10 cards");
        expect(isStarterDeckTemplateValid(starterDeck), "starter deck composition valid");
        expect(hasUniqueRelics({RelicType::EmberCharm, RelicType::IronSigil, RelicType::BloodVial}), "unique relic list valid");
        expect(!hasUniqueRelics({RelicType::EmberCharm, RelicType::EmberCharm}), "duplicate relic list rejected");
        expect(relicThornDamage(true) == 3, "thorn relic retaliation tuned");
        expect(relicThornDamage(false) == 0, "thorn retaliation absent without relic");
        expect(relicBattleStartHeal(true) == 5, "blood vial heal tuned");
        expect(relicBattleStartHeal(false) == 0, "blood vial absent no heal");
        expect(relicTurnStartEnergyBonus(0, true) == 1, "ember first turn bonus active");
        expect(relicTurnStartEnergyBonus(1, true) == 0, "ember later turns no bonus");
        expect(relicTurnStartDrawBonus(true) == 1, "tactician draw bonus active");
        expect(relicTurnStartDrawBonus(false) == 0, "tactician draw bonus absent");

        out << (success ? "SELF_TEST_PASS" : "SELF_TEST_FAIL") << '\n';
        return success;
    }

    Game()
        : window(sf::VideoMode(1280, 720), sf::String(L"尖塔战士")),
          rng(static_cast<std::mt19937::result_type>(std::random_device{}())) {
                loadConfig();
                enableCombatAnimations = config.enableAnimations;
                window.setFramerateLimit(config.maxFps);
        loadFont();
        loadTextures();
        initCards();
        initEnemies();
        if (!validateGameData(std::cerr)) {
            throw std::runtime_error("Invalid game data detected during startup");
        }
        loadProgress();
        phase = Phase::MainMenu;
    }

    void run() {
        sf::Clock frameClock;
        while (window.isOpen()) {
            const float dt = frameClock.restart().asSeconds();
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                handleEvent(event);
            }
            update(dt);
            render();
        }
    }

private:
    // ========================================================================
    // SFML 图形资源
    // ========================================================================
    sf::RenderWindow window;        // 主窗口（1280x720）
    sf::Font font;                  // 主字体
    bool hasFont = false;           // 字体加载状态
    std::string loadedFontPath;     // 已加载的字体路径

    // 卡牌类型纹理
    sf::Texture cardAttackTex;      // 攻击牌背景
    sf::Texture cardSkillTex;       // 技能牌背景
    sf::Texture cardPowerTex;       // 能力牌背景
    
    // 敌人立绘纹理
    sf::Texture enemy1Tex;          // 敌人1：狡猾狐狸
    sf::Texture enemy2Tex;          // 敌人2：邪教徒
    sf::Texture enemy3Tex;          // 敌人3：剧毒曼巴
    sf::Texture enemy4Tex;          // 敌人4：刀盾卫兵
    sf::Texture elite1Tex;          // 精英1：饿狼首领
    sf::Texture elite2Tex;          // 精英2：鲜血斗士
    sf::Texture elite3Tex;          // 精英3：诅咒祭司
    sf::Texture eliteTex;           // 精英通用图标
    sf::Texture bossTex;            // Boss：尖塔守卫
    
    // UI元素纹理
    sf::Texture logoTex;            // 游戏Logo
    sf::Texture intentTex;          // 敌人意图图标
    sf::Texture startTex;           // 开始按钮图标
    sf::Texture exitTex;            // 退出按钮图标
    sf::Texture thanksTex;          // 感谢页面图标
    sf::Texture uiGoldTex;          // 金币图标
    sf::Texture uiPotionTex;        // 药水图标
    sf::Texture uiDeckTex;          // 牌组图标
    sf::Texture uiRelicTex;         // 遗物图标
    
    // 状态图标纹理
    sf::Texture statHpTex;          // 生命值图标
    sf::Texture statEnergyTex;      // 能量图标
    sf::Texture statBlockTex;       // 格挡图标
    sf::Texture statStrengthTex;    // 力量图标

    // 场景背景纹理
    sf::Texture bgMainMenu;         // 主菜单背景
    sf::Texture bgBattle;           // 战斗背景
    sf::Texture bgCampfire;         // 营火背景
    sf::Texture bgShop;             // 商店背景
    sf::Texture bgEvent1;           // 事件1背景：神殿祭坛
    sf::Texture bgEvent2;           // 事件2背景：森林小路
    sf::Texture bgDefeat;           // 战败背景
    sf::Texture bgVictory;          // 胜利背景

    // ========================================================================
    // 音频资源
    // ========================================================================
    std::string currentBgmPath;     // 当前播放的BGM路径
    SfxBuffer sfxSwordBuf;          // 剑攻击音效
    SfxBuffer sfxPlayerHitBuf;      // 玩家受击音效
    SfxBuffer sfxEnemyHitBuf;       // 敌人受击音效
    SfxBuffer sfxCardPlayBuf;       // 打出卡牌音效
    SfxBuffer sfxBlockBuf;          // 格挡音效
    SfxBuffer sfxHealBuf;           // 治疗音效
    SfxBuffer sfxDefeatBuf;         // 战败音效
    SfxBuffer sfxVictoryBuf;        // 胜利音效
    SfxBuffer sfxUiClickBuf;        // UI点击音效
    float sfxCooldown = 0.0f;       // 音效冷却时间（防止重叠播放）

#if USE_AUDIO
    sf::Music bgmPlayer;            // BGM播放器
    sf::Sound sfxPlayer;            // 音效播放器
#endif

    // ========================================================================
    // 随机数生成器
    // ========================================================================
    std::mt19937 rng;               // Mersenne Twister随机数引擎

    // ========================================================================
    // 游戏数据容器
    // ========================================================================
    std::vector<CardDef> cardPool;              // 所有可用卡牌定义
    std::unordered_map<int, CardDef> cardsById; // 卡牌ID到定义的映射（O(1)查找）
    std::vector<int> baseCardIds;               // 基础卡牌ID列表（不含升级版）
    std::vector<EnemyDef> enemySequence;        // 敌人序列（按出现顺序）
    std::vector<FloorNode> floorPlan;           // 地图楼层配置
    std::vector<int> shopChoices;               // 商店可选卡牌ID

    // ========================================================================
    // 游戏状态变量
    // ========================================================================
    Phase phase = Phase::MainMenu;      // 当前游戏阶段
    int currentFloor = 0;               // 当前楼层（0-14）
    int currentEnemyIndex = 0;          // 当前敌人索引
    int currentEventId = 0;             // 当前事件ID
    float phaseTimer = 0.0f;            // 阶段计时器（用于过渡动画）

    // ========================================================================
    // 战斗状态
    // ========================================================================
    Combatant player;                   // 玩家状态
    Combatant enemy;                    // 敌人状态
    int enemyIntentIndex = 0;           // 敌人当前意图索引（循环模式）

    int playerEnergy = Balance::kBaseTurnEnergy;    // 当前能量
    int playerStrengthPerTurn = 0;                  // 每回合获得的力量（薪火之源效果）
    int playerEnergyPerTurn = 0;                    // 每回合获得的能量
    int totalWins = 0;                              // 累计通关次数（持久化存储）
    int gold = Balance::kStartingGold;              // 当前金币
    int potionCount = Balance::kStartingPotionCount;// 当前药水数量
    int potionMax = Balance::kStartingPotionMax;    // 药水上限
    int potionCapacityPrice = Balance::kStartingPotionCapacityPrice; // 购买药水价格
    int deckRemovePrice = Balance::kStartingDeckRemovePrice;         // 删除卡牌价格
    bool potionPurchasedThisRun = false;            // 本场是否已购买药水（限购1瓶）
    int eliteRelicDropChance = Balance::kEliteRelicDropChance;       // 精英掉落遗物概率
    bool campfireDidAction = false;                 // 营火是否已执行动作
    DeckEditMode deckEditMode = DeckEditMode::None; // 牌组编辑模式
    int deckEditRemaining = 0;                      // 剩余编辑次数
    int rampageDamageBonus = 0;                     // 暴走累计伤害加成
    bool blockPersists = false;                     // 格挡是否持续（壁垒效果）
    bool darkEmbraceActive = false;                 // 黑暗之拥是否激活
    bool tearActive = false;                        // 撕裂是否激活

    // ========================================================================
    // 牌组与牌堆
    // ========================================================================
    std::vector<int> masterDeck;    // 主牌组（本场游戏的所有卡牌）
    std::vector<int> drawPile;      // 抽牌堆
    std::vector<int> discardPile;   // 弃牌堆
    std::vector<int> hand;          // 手牌
    std::vector<int> exhaustPile;   // 消耗堆

    // ========================================================================
    // 奖励与遗物
    // ========================================================================
    std::vector<int> rewardChoices;             // 奖励卡牌选项
    int eliteRewardPicksRemaining = 0;          // 精英奖励剩余选择次数
    std::vector<RelicType> relics;              // 已获得的遗物列表
    std::deque<std::string> logs;               // 战斗日志（双端队列，限制长度）
    std::vector<FloatingText> floatingTexts;    // 浮动文字效果列表
    PendingCardPlay pendingCard;                // 待处理的卡牌动画
    PendingEnemyTurn pendingEnemy;              // 待处理的敌人回合动画
    AppConfig config;                           // 应用配置

    sf::FloatRect endTurnRect{1146.f, 530.f, 94.f, 40.f};
    sf::FloatRect potionRect{1168.f, 422.f, 102.f, 52.f};
    sf::FloatRect menuStartRect{470.f, 380.f, 340.f, 82.f};
    sf::FloatRect menuRulesRect{470.f, 480.f, 340.f, 82.f};
    sf::FloatRect menuExitRect{470.f, 580.f, 340.f, 82.f};
    sf::FloatRect backMenuRect{470.f, 560.f, 340.f, 72.f};
    sf::FloatRect campRestRect{270.f, 320.f, 320.f, 90.f};
    sf::FloatRect campForgeRect{700.f, 320.f, 320.f, 90.f};
    sf::FloatRect shopPotionRect{940.f, 500.f, 280.f, 70.f};
    sf::FloatRect shopRemoveRect{940.f, 590.f, 280.f, 70.f};
    sf::FloatRect shopLeaveRect{940.f, 410.f, 280.f, 70.f};
    sf::FloatRect eventLeftRect{180.f, 500.f, 430.f, 120.f};
    sf::FloatRect eventRightRect{670.f, 500.f, 430.f, 120.f};
    sf::FloatRect deckDoneRect{1000.f, 620.f, 230.f, 70.f};
    sf::FloatRect runStatsRect{902.f, 14.f, 350.f, 108.f};
    sf::FloatRect ownedDeckCloseRect{1050.f, 120.f, 180.f, 42.f};
    sf::FloatRect relicInspectRect{1084.f, 104.f, 166.f, 22.f};
    sf::FloatRect relicOverlayCloseRect{1040.f, 122.f, 190.f, 42.f};
    sf::FloatRect drawPileRect{20.f, 584.f, 102.f, 52.f};
    sf::FloatRect discardPileRect{1168.f, 300.f, 102.f, 52.f};
    sf::FloatRect exhaustPileRect{1168.f, 360.f, 102.f, 52.f};
    sf::FloatRect pileOverlayCloseRect{930.f, 130.f, 190.f, 42.f};
    sf::FloatRect devTestRect{1220.f, 680.f, 40.f, 20.f};
    sf::FloatRect bgmToggleRect{1170.f, 680.f, 40.f, 20.f};

    float enemyHitFlashTimer = 0.0f;
    float playerHitFlashTimer = 0.0f;
    float turnBannerTimer = 0.0f;
    int hoveredCardIndex = -1;
    bool enableCombatAnimations = true;
    Phase deckEditReturnPhase = Phase::MainMenu;
    bool deckEditAdvanceAfterDone = false;
    bool showOwnedDeckOverlay = false;
    bool showRelicOverlay = false;
    bool bgmEnabled = true;
    float bgmVolume = 20.f;
    PileOverlayType activePileOverlay = PileOverlayType::None;
    std::string battleQuoteText;
    float battleQuoteTimer = 0.0f;
    int battleTurnCount = 0;
    std::string actionHint;
    sf::Color actionHintColor = sf::Color(240, 230, 180);
    float actionHintTimer = 0.0f;

    static constexpr const char* kSavePath = "save_progress.dat";
    static constexpr const char* kConfigPath = "game_config.ini";

    void showActionHint(const std::string& msg, const sf::Color& color = sf::Color(240, 230, 180), float duration = 2.3f) {
        actionHint = msg;
        actionHintColor = color;
        actionHintTimer = duration;
    }

    bool hasUpgradableCardInDeck() const {
        for (int id : masterDeck) {
            if (isUpgradedCard(id)) {
                continue;
            }
            if (findCard(upgradedCardId(id)) != nullptr) {
                return true;
            }
        }
        return false;
    }

    static std::string trim(const std::string& s) {
        std::size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
            ++start;
        }
        std::size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
            --end;
        }
        return s.substr(start, end - start);
    }

    static bool parseBool(const std::string& value) {
        std::string lowered = value;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
    }

    static void applyConfigEntry(AppConfig& target, const std::string& key, const std::string& value, std::ostream* warnOut) {
        try {
            if (key == "max_fps") {
                const int parsed = std::stoi(value);
                target.maxFps = static_cast<unsigned int>(std::clamp(parsed, 30, 240));
            } else if (key == "enable_animations") {
                target.enableAnimations = parseBool(value);
            } else if (key == "enable_floating_text") {
                target.enableFloatingText = parseBool(value);
            } else if (key == "text_scale") {
                const float parsed = std::stof(value);
                target.textScale = std::clamp(parsed, 0.8f, 1.5f);
            }
        } catch (...) {
            if (warnOut != nullptr) {
                *warnOut << "[warn] 配置项解析失败: " << key << "=" << value << '\n';
            }
        }
    }

    static AppConfig parseConfigLines(const std::vector<std::string>& lines, std::ostream* warnOut) {
        AppConfig parsed;
        for (const auto& line : lines) {
            const std::string raw = trim(line);
            if (raw.empty() || raw[0] == '#') {
                continue;
            }

            const std::size_t pos = raw.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            const std::string key = trim(raw.substr(0, pos));
            const std::string value = trim(raw.substr(pos + 1));
            applyConfigEntry(parsed, key, value, warnOut);
        }
        return parsed;
    }

    void writeDefaultConfig() const {
        std::ofstream out(kConfigPath, std::ios::trunc);
        if (!out) {
            return;
        }
        out << "# MiniSpireSFML runtime config\n";
        out << "max_fps=60\n";
        out << "enable_animations=true\n";
        out << "enable_floating_text=true\n";
        out << "text_scale=1.0\n";
    }

    void loadConfig() {
        std::ifstream in(kConfigPath);
        if (!in) {
            writeDefaultConfig();
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }

        config = parseConfigLines(lines, &std::cerr);
    }

    static bool isValidTransition(Phase from, Phase to) {
        if (from == to) {
            return true;
        }
        switch (from) {
            case Phase::MainMenu: return to == Phase::Battle || to == Phase::Campfire || to == Phase::Shop || to == Phase::Event;
            case Phase::Battle: return to == Phase::Reward || to == Phase::Defeat || to == Phase::VictoryThanks || to == Phase::MainMenu;
            case Phase::Reward: return to == Phase::Battle || to == Phase::Campfire || to == Phase::Shop || to == Phase::Event || to == Phase::MainMenu;
            case Phase::Campfire: return to == Phase::DeckEdit || to == Phase::Battle || to == Phase::Shop || to == Phase::Event || to == Phase::VictoryThanks;
            case Phase::Shop: return to == Phase::DeckEdit || to == Phase::Battle || to == Phase::Campfire || to == Phase::Event || to == Phase::VictoryThanks;
            case Phase::Event: return to == Phase::DeckEdit || to == Phase::Battle || to == Phase::Campfire || to == Phase::Shop || to == Phase::VictoryThanks;
            case Phase::DeckEdit: return to == Phase::Campfire || to == Phase::Shop || to == Phase::Event || to == Phase::MainMenu;
            case Phase::VictoryThanks: return to == Phase::Credits;
            case Phase::Credits: return to == Phase::MainMenu;
            case Phase::Defeat: return to == Phase::MainMenu || to == Phase::Battle;
            default: return false;
        }
    }

    void transitionTo(Phase to, const std::string& reason) {
        if (!isValidTransition(phase, to)) {
            std::cerr << "[warn] 非法阶段跳转: " << static_cast<int>(phase) << " -> " << static_cast<int>(to)
                      << " 原因: " << reason << '\n';
            return;
        }
        phase = to;
        if (to == Phase::Defeat) {
            playSfx(sfxDefeatBuf, 70.f);
        } else if (to == Phase::VictoryThanks) {
            playSfx(sfxVictoryBuf, 70.f);
        }
    }

    void loadFont() {
        const std::vector<std::string> candidates = {
            // Game-style CJK fonts (if user places them in assets/fonts)
            "assets/fonts/ZCOOLKuaiLe-Regular.ttf",
            "assets/fonts/ZCOOLQingKeHuangYou-Regular.ttf",
            "assets/fonts/SmileySans-Oblique.ttf",
            // Stable bundled fallback
            "assets/fonts/NotoSansCJKsc-Regular.otf",
            // Legacy/local fallback
            "assets/font.ttf",
            // Windows CJK/system fallbacks
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };

        hasFont = false;
        loadedFontPath.clear();
        for (const auto& path : candidates) {
            if (font.loadFromFile(path)) {
                hasFont = true;
                loadedFontPath = path;
                break;
            }
        }

        if (!hasFont) {
            std::cerr << "[warn] 字体加载失败，文本将无法显示" << '\n';
        } else {
            std::cerr << "[info] 字体加载成功: " << loadedFontPath << '\n';
        }
    }

    void loadTexture(sf::Texture& tex, const std::string& path) {
        if (!tex.loadFromFile(path)) {
            std::cerr << "[warn] 纹理加载失败: " << path << '\n';
        }
    }

    void spawnFloatingText(const std::string& text, const sf::Vector2f& anchor, const sf::Color& color, float duration = 0.75f) {
        if (!config.enableFloatingText) {
            return;
        }
        std::uniform_real_distribution<float> xJitter(-18.f, 18.f);
        FloatingText item;
        item.text = sf::String::fromUtf8(text.begin(), text.end());
        item.position = sf::Vector2f(anchor.x + xJitter(rng), anchor.y);
        item.velocity = sf::Vector2f(0.f, -48.f);
        item.color = color;
        item.remaining = duration;
        item.duration = duration;
        floatingTexts.push_back(item);
    }

    void loadTextures() {
        loadTexture(cardAttackTex, "assets/images/cards/attack.png");
        loadTexture(cardSkillTex, "assets/images/cards/skill.png");
        loadTexture(cardPowerTex, "assets/images/cards/power.png");

        loadTexture(enemy1Tex, "assets/images/enemies/enemy1.png");
        loadTexture(enemy2Tex, "assets/images/enemies/enemy2.png");
        loadTexture(enemy3Tex, "assets/images/enemies/enemy3.png");
        loadTexture(enemy4Tex, "assets/images/enemies/enemy4.png");
        loadTexture(elite1Tex, "assets/images/enemies/elite1.png");
        loadTexture(elite2Tex, "assets/images/enemies/elite2.png");
        loadTexture(elite3Tex, "assets/images/enemies/elite3.png");
        loadTexture(eliteTex, "assets/images/enemies/elite.png");
        loadTexture(bossTex, "assets/images/enemies/boss.png");

        loadTexture(logoTex, "assets/images/ui/logo.png");
        loadTexture(intentTex, "assets/images/ui/intent.png");
        loadTexture(startTex, "assets/images/ui/start.png");
        loadTexture(exitTex, "assets/images/ui/exit.png");
        loadTexture(thanksTex, "assets/images/ui/thanks.png");
        loadTexture(uiGoldTex, "assets/images/ui/stat_gold.png");
        loadTexture(uiPotionTex, "assets/images/ui/stat_potion.png");
        loadTexture(uiDeckTex, "assets/images/ui/stat_deck.png");
        loadTexture(uiRelicTex, "assets/images/ui/stat_relic.png");
        loadTexture(statHpTex, "assets/images/ui/stat_hp.png");
        loadTexture(statEnergyTex, "assets/images/ui/stat_energy.png");
        loadTexture(statBlockTex, "assets/images/ui/stat_block.png");
        loadTexture(statStrengthTex, "assets/images/ui/stat_strength.png");

        loadTexture(bgMainMenu, "assets/images/backgrounds/main_menu.png");
        loadTexture(bgBattle, "assets/images/backgrounds/battle.png");
        loadTexture(bgCampfire, "assets/images/backgrounds/campfire.png");
        loadTexture(bgShop, "assets/images/backgrounds/shop.png");
        loadTexture(bgEvent1, "assets/images/backgrounds/event1.png");
        loadTexture(bgEvent2, "assets/images/backgrounds/event2.png");
        loadTexture(bgDefeat, "assets/images/backgrounds/defeat.png");
        loadTexture(bgVictory, "assets/images/backgrounds/victory.png");

#if USE_AUDIO
        loadSfxBuffer(sfxSwordBuf, "assets/audio/sfx/sword_attack.mp3");
        loadSfxBuffer(sfxPlayerHitBuf, "assets/audio/sfx/player_hit.mp3");
        loadSfxBuffer(sfxEnemyHitBuf, "assets/audio/sfx/enemy_hit.mp3");
        loadSfxBuffer(sfxCardPlayBuf, "assets/audio/sfx/card_play.mp3");
        loadSfxBuffer(sfxBlockBuf, "assets/audio/sfx/block.mp3");
        loadSfxBuffer(sfxHealBuf, "assets/audio/sfx/heal.mp3");
        loadSfxBuffer(sfxDefeatBuf, "assets/audio/sfx/defeat.mp3");
        loadSfxBuffer(sfxVictoryBuf, "assets/audio/sfx/victory.mp3");
        loadSfxBuffer(sfxUiClickBuf, "assets/audio/sfx/card_play.mp3");
#endif
    }

#if USE_AUDIO
    void loadSfxBuffer(SfxBuffer& buf, const std::string& path) {
        if (!buf.loadFromFile(path)) {
            std::cerr << "[warn] 音效加载失败: " << path << '\n';
        }
    }

    std::string resolveBgmPath(const std::string& preferredPath) const {
        namespace fs = std::filesystem;
        std::string basePath = preferredPath;
        if (preferredPath.size() > 4 && preferredPath.substr(preferredPath.size() - 4) == ".mp3") {
            basePath = preferredPath.substr(0, preferredPath.size() - 4);
        }
        const std::string wavPath = basePath + ".wav";
        if (fs::exists(wavPath)) {
            return wavPath;
        }
        const std::string mp3Path = basePath + ".mp3";
        if (fs::exists(mp3Path)) {
            return mp3Path;
        }
        const std::string oggPath = basePath + ".ogg";
        if (fs::exists(oggPath)) {
            return oggPath;
        }
        return preferredPath;
    }

    void playBgm(const std::string& bgmPath) {
        const std::string resolvedPath = resolveBgmPath(bgmPath);
        if (resolvedPath == currentBgmPath && bgmPlayer.getStatus() == sf::Music::Playing) return;
        currentBgmPath = resolvedPath;
        if (bgmPlayer.openFromFile(resolvedPath)) {
            bgmPlayer.setLoop(true);
            bgmPlayer.setVolume(bgmEnabled ? bgmVolume : 0.f);
            bgmPlayer.play();
        } else {
            std::cerr << "[warn] BGM加载失败: " << bgmPath << '\n';
        }
    }

    void toggleBgm() {
        bgmEnabled = !bgmEnabled;
        bgmPlayer.setVolume(bgmEnabled ? bgmVolume : 0.f);
    }

    void stopBgm() {
        bgmPlayer.stop();
        currentBgmPath.clear();
    }

    void playSfx(const SfxBuffer& buf, float volume = 60.f) {
        if (sfxCooldown > 0.0f) return;
        sfxPlayer.setBuffer(buf);
        sfxPlayer.setVolume(volume);
        sfxPlayer.play();
        sfxCooldown = 0.05f;
    }
#else
    void playBgm(const std::string& bgmPath) { currentBgmPath = bgmPath; }
    void stopBgm() { currentBgmPath.clear(); }
    void playSfx(const SfxBuffer&, float = 60.f) {}
#endif

    void pushLog(const std::string& line) {
        logs.push_back(line);
        while (logs.size() > 12) {
            logs.pop_front();
        }
    }

    std::string storylineIntro() const {
        return "火焰高塔从未熄灭。每一层都在低语，诱你献出血、钢与记忆。";
    }

    std::string storylineGoal() const {
        return "你是最后一名攀塔者。穿过15层，击败尖塔守卫，让余烬再次点亮王城。";
    }

    std::string enemyEntranceQuote(int enemyIndex) const {
        switch (enemyIndex) {
            case 0: return "狡猾狐狸：嘿嘿，来抓我呀！";
            case 1: return "邪教徒：尖塔将吞下你的名字。";
            case 2: return "剧毒曼巴：嘶嘶...毒液已准备好。";
            case 3: return "刀盾卫兵：此路到此为止。";
            case 4: return "饿狼首领：嗷呜！我的獠牙会撕裂你！";
            case 5: return "鲜血斗士：鲜血，是唯一的誓言。";
            case 6: return "诅咒祭司：诅咒会比你先抵达终点。";
            case 7: return "尖塔守卫：你的火焰很亮，但终会熄灭。";
            default: return "未知敌人：......";
        }
    }

    static std::string relicName(RelicType relic) {
        switch (relic) {
            case RelicType::EmberCharm: return "余烬护符";
            case RelicType::IronSigil: return "铁躯纹章";
            case RelicType::BloodVial: return "血瓶";
            case RelicType::TacticianLens: return "战术透镜";
            case RelicType::ThornEmblem: return "荆棘徽记";
            default: return "未知遗物";
        }
    }

    static std::string relicDesc(RelicType relic) {
        switch (relic) {
            case RelicType::EmberCharm: return "首回合开始时额外获得 1 点能量";
            case RelicType::IronSigil: return "获得时 +8 最大生命并回复 8 点生命";
            case RelicType::BloodVial: return "每场战斗开始时回复 5 点生命";
            case RelicType::TacticianLens: return "每回合额外抽 1 张牌";
            case RelicType::ThornEmblem: return "受到攻击时反伤 3";
            default: return "";
        }
    }

    static int relicBattleStartHeal(bool hasBloodVial) {
        return hasBloodVial ? Balance::kBloodVialHeal : 0;
    }

    static int relicTurnStartEnergyBonus(int battleTurnIndex, bool hasEmberCharm) {
        return (hasEmberCharm && battleTurnIndex == 0) ? 1 : 0;
    }

    static int relicTurnStartDrawBonus(bool hasTacticianLens) {
        return hasTacticianLens ? 1 : 0;
    }

    static std::vector<int> starterDeckTemplate() {
        return {1, 1, 1, 1, 2, 2, 2, 2, 3, 21};
    }

    static bool isStarterDeckTemplateValid(const std::vector<int>& deck) {
        if (deck.size() != 10u) {
            return false;
        }
        std::unordered_map<int, int> counts;
        for (int id : deck) {
            counts[id] += 1;
        }
        return counts[1] == 4 && counts[2] == 4 && counts[3] == 1 && counts[21] == 1;
    }

    static bool hasUniqueRelics(const std::vector<RelicType>& relicList) {
        for (size_t i = 0; i < relicList.size(); ++i) {
            for (size_t j = i + 1; j < relicList.size(); ++j) {
                if (relicList[i] == relicList[j]) {
                    return false;
                }
            }
        }
        return true;
    }

    static int relicThornDamage(bool hasThornRelic) {
        return hasThornRelic ? 3 : 0;
    }

    bool hasRelic(RelicType relic) const {
        return std::find(relics.begin(), relics.end(), relic) != relics.end();
    }

    static bool validateCardDefBasic(const CardDef& card, std::ostream& out) {
        if (card.cost < 0 || card.cost > 6) {
            out << "[error] invalid card cost id=" << card.id << " cost=" << card.cost << '\n';
            return false;
        }
        if (card.hits <= 0 || card.hits > 10) {
            out << "[error] invalid card hits id=" << card.id << " hits=" << card.hits << '\n';
            return false;
        }
        return true;
    }

    static bool validateEnemyDefBasic(const EnemyDef& enemyDef, size_t index, std::ostream& out) {
        if (enemyDef.maxHp <= 0) {
            out << "[error] invalid enemy hp index=" << index << " hp=" << enemyDef.maxHp << '\n';
            return false;
        }
        if (enemyDef.pattern.empty()) {
            out << "[error] enemy has empty intent pattern index=" << index << '\n';
            return false;
        }
        return true;
    }

    static bool validateFloorNodeBasic(const FloorNode& node, size_t floorIndex, int enemyCount, std::ostream& out) {
        if (node.type == RoomType::Battle) {
            if (node.enemyIndex < 0 || node.enemyIndex >= enemyCount) {
                out << "[error] floor plan battle enemy index out of range floor=" << floorIndex
                    << " enemyIndex=" << node.enemyIndex << '\n';
                return false;
            }
        }
        if (node.type == RoomType::Event) {
            if (node.eventId != 1 && node.eventId != 2) {
                out << "[error] floor plan invalid event id floor=" << floorIndex
                    << " eventId=" << node.eventId << '\n';
                return false;
            }
        }
        return true;
    }

    bool validateCardPoolData(std::ostream& out) const {
        std::unordered_set<int> ids;
        for (const auto& card : cardPool) {
            if (!ids.insert(card.id).second) {
                out << "[error] duplicate card id: " << card.id << '\n';
                return false;
            }
            if (!validateCardDefBasic(card, out)) {
                return false;
            }
        }
        return true;
    }

    bool validateEnemyData(std::ostream& out) const {
        if (enemySequence.empty()) {
            out << "[error] enemy sequence is empty" << '\n';
            return false;
        }
        for (size_t i = 0; i < enemySequence.size(); ++i) {
            if (!validateEnemyDefBasic(enemySequence[i], i, out)) {
                return false;
            }
        }
        return true;
    }

    bool validateFloorPlanData(std::ostream& out) const {
        if (floorPlan.empty()) {
            out << "[error] floor plan is empty" << '\n';
            return false;
        }
        for (size_t i = 0; i < floorPlan.size(); ++i) {
            if (!validateFloorNodeBasic(floorPlan[i], i, static_cast<int>(enemySequence.size()), out)) {
                return false;
            }
        }
        return true;
    }

    bool validateGameData(std::ostream& out) const {
        return validateCardPoolData(out) && validateEnemyData(out) && validateFloorPlanData(out);
    }

    std::string relicSummaryShort() const {
        if (relics.empty()) {
            return "无";
        }
        std::string summary;
        const int showCount = std::min(2, static_cast<int>(relics.size()));
        for (int i = 0; i < showCount; ++i) {
            if (!summary.empty()) {
                summary += "、";
            }
            summary += relicName(relics[i]);
        }
        if (static_cast<int>(relics.size()) > showCount) {
            summary += "...";
        }
        return summary;
    }

    void grantRelic(RelicType relic) {
        if (hasRelic(relic)) {
            return;
        }

        relics.push_back(relic);
        pushLog("获得遗物：" + relicName(relic));
        pushLog("遗物效果：" + relicDesc(relic));
        showActionHint("获得遗物：" + relicName(relic), sf::Color(255, 226, 150));

        if (relic == RelicType::IronSigil) {
            player.maxHp += 8;
            player.hp = std::min(player.maxHp, player.hp + 8);
            pushLog("铁躯纹章生效：最大生命 +8，回复 8 点生命");
        }
    }

    bool tryGrantEliteRelicDrop() {
        std::uniform_int_distribution<int> rollDist(1, 100);
        if (rollDist(rng) > eliteRelicDropChance) {
            return false;
        }

        std::vector<RelicType> candidates = {
            RelicType::EmberCharm,
            RelicType::IronSigil,
            RelicType::BloodVial,
            RelicType::TacticianLens,
            RelicType::ThornEmblem
        };

        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(), [&](RelicType relic) { return hasRelic(relic); }),
            candidates.end());

        if (candidates.empty()) {
            return false;
        }

        std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);
        grantRelic(candidates[dist(rng)]);
        return true;
    }

    void initCards() {
        cardsById.clear();
        cardPool.clear();
        baseCardIds.clear();

        cardPool = {
            {1, "打击", CardType::Attack, 1, 6, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 6 点伤害"},
            {2, "防御", CardType::Skill, 1, 0, 1, 5, 0, 0, 0, 0, 0, 0, false, false, "获得 5 点格挡"},
            {3, "重击", CardType::Attack, 2, 8, 1, 0, 0, 2, 0, 0, 0, 0, false, false, "造成 8 点伤害，施加 2 层易伤"},
            {6, "怒火", CardType::Skill, 1, 0, 1, 0, 2, 0, 0, 0, 0, 0, false, false, "获得 2 点力量"},
            {11, "铁波斩", CardType::Attack, 1, 7, 1, 7, 0, 0, 0, 0, 0, 0, false, false, "造成 7 点伤害并获得 7 点格挡"},
            {12, "强行防御", CardType::Skill, 1, 0, 1, 14, 0, 0, 0, 0, 0, 0, false, false, "获得 14 点格挡"},
            {13, "耸肩无视", CardType::Skill, 1, 0, 1, 10, 0, 0, 0, 1, 0, 0, false, false, "获得 10 点格挡，抽 1 张牌"},
            {14, "上勾拳", CardType::Attack, 2, 10, 1, 0, 0, 2, 2, 0, 0, 0, false, false, "造成 10 点伤害，施加 2 层虚弱和 2 层易伤"},
            {15, "乱剑雨", CardType::Attack, 1, 4, 3, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 4 点伤害，共 3 次"},
            {16, "极限突破", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "将你的力量翻倍，消耗"},
            {17, "暴走", CardType::Attack, 1, 10, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 10 点伤害，本场战斗中每打出一次伤害+8"},
            {18, "献祭", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 5, 2, 6, true, false, "失去 6 点生命，获得 2 点能量并抽 5 张牌，消耗"},
            {19, "壁垒", CardType::Skill, 2, 0, 1, 12, 0, 0, 0, 0, 0, 0, true, false, "获得 12 点格挡，本场战斗中格挡不再在回合结束时消失，消耗"},
            {20, "恶魔形态", CardType::Power, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, true, "每回合开始获得 3 点力量，消耗"},

            {21, "快速检索", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, true, false, "抽 1 张牌，消耗"},
            {22, "战术研读", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 2, 0, 0, false, false, "抽 2 张牌"},
            {24, "冲刺", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 3, 0, 0, true, false, "抽 3 张牌，消耗"},
            {25, "窥探弱点", CardType::Skill, 1, 0, 1, 0, 0, 2, 0, 2, 0, 0, false, false, "施加 2 层易伤并抽 2 张牌"},
            {27, "潦草急就", CardType::Skill, 2, 0, 1, 0, 0, 0, 0, 6, 0, 0, true, false, "抽 6 张牌，消耗"},
            {28, "应急补给", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, true, false, "抽 1 张牌并获得 1 点能量，消耗"},

            {29, "战斗专注", CardType::Power, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, true, true, "每回合开始获得 1 点力量，消耗"},
            {32, "双重格挡", CardType::Skill, 2, 0, 1, 18, 0, 0, 0, 0, 0, 0, true, false, "获得 18 点格挡，消耗"},
            {33, "痛击", CardType::Attack, 2, 16, 1, 0, 0, 1, 0, 0, 0, 0, true, false, "造成 16 点伤害并施加 1 层易伤，消耗"},
            {36, "邪恶之刃", CardType::Attack, 2, 8, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 8 点伤害并回复等量生命", true, false, false, true},
            {37, "放血", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 0, 2, 3, false, false, "失去 3 点生命，获得 2 点能量"},
            {38, "盛怒", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, true, false, "获得 2 点能量，消耗"},
            {39, "薪火之源", CardType::Power, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "每回合开始时获得 1 点能量，消耗"},
            {40, "添柴", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "消耗手牌中其余所有卡牌，然后抽取等量卡牌"},
            {41, "重振精神", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "消耗手牌中所有非攻击牌，每张获得 7 点格挡"},
            {42, "战鼓", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "消耗手牌中所有攻击牌，每张造成 7 点伤害"},
            {43, "黑暗之拥", CardType::Power, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "本场战斗中，每当你消耗一张牌时抽一张牌，消耗"},
            {44, "剑柄打击", CardType::Attack, 1, 10, 1, 0, 0, 0, 0, 2, 0, 0, false, false, "造成 10 点伤害，抽 2 张牌"},

            {45, "全身撞击", CardType::Attack, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成等同于你格挡值的伤害"},
            {46, "巩固", CardType::Skill, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "将你的格挡值翻倍"},
            {47, "威吓", CardType::Skill, 1, 0, 1, 0, 0, 3, 0, 0, 0, 0, true, false, "给敌人 3 层虚弱，消耗"},
            {48, "撕裂", CardType::Power, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "本场战斗中每次失去生命时获得 1 点力量，消耗"},
            {49, "震荡波", CardType::Skill, 2, 0, 1, 0, 0, 5, 0, 0, 0, 0, true, false, "给敌人 5 层易伤和 5 层虚弱，消耗"},
            {50, "浴血术", CardType::Attack, 2, 22, 1, 0, 0, 0, 0, 0, 0, 3, false, false, "失去 3 点生命，造成 22 点伤害"}
        };

        for (auto& c : cardPool) {
            if (c.id == 36) {
                c.healFromDamage = true;
            }
            if (c.id == 39) {
                c.grantsEnergyPerTurn = true;
            }
            if (c.id == 40) {
                c.redrawByExhaustingHand = true;
            }
        }

        std::vector<CardDef> upgradedCards;
        upgradedCards.reserve(cardPool.size());
        for (const auto& base : cardPool) {
            baseCardIds.push_back(base.id);
            upgradedCards.push_back(buildUpgradedCard(base));
        }

        for (const auto& up : upgradedCards) {
            cardPool.push_back(up);
        }

        for (const auto& c : cardPool) {
            cardsById[c.id] = c;
        }
    }

    void initEnemies() {
        enemySequence.clear();

        enemySequence.push_back({
            "普通敌人：狡猾狐狸", 45, false, false,
            {{"攻击 9", 9, 1, 0, 0, 0, 0}, {"格挡 8 + 攻击 6", 6, 1, 8, 0, 0, 0}, {"攻击 9", 9, 1, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：邪教徒", 60, false, false,
            {{"强化 +2 力量", 0, 1, 0, 2, 0, 0}, {"攻击 8 x2", 8, 2, 0, 0, 0, 0}, {"攻击 12", 12, 1, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：剧毒曼巴", 65, false, false,
            {{"施加虚弱 1 + 攻击 8", 8, 1, 0, 0, 1, 0}, {"攻击 10", 10, 1, 0, 0, 0, 0}, {"格挡 10", 0, 1, 10, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：刀盾卫兵", 70, false, false,
            {{"攻击 10", 10, 1, 0, 0, 0, 0}, {"格挡 12", 0, 1, 12, 0, 0, 0}, {"攻击 8 x2", 8, 2, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "精英敌人：饿狼首领", 95, true, false,
            {{"格挡 12 + 强化 +1 力量", 0, 1, 12, 1, 0, 0}, {"攻击 14", 14, 1, 0, 0, 0, 0}, {"攻击 9 x2", 9, 2, 0, 0, 0, 0}, {"施加易伤 1 + 攻击 10", 10, 1, 0, 0, 0, 1}}});

        enemySequence.push_back({
            "精英敌人：鲜血斗士", 120, true, false,
            {{"强化 +3 力量", 0, 1, 0, 3, 0, 0}, {"攻击 18", 18, 1, 0, 0, 0, 0}, {"攻击 11 x2", 11, 2, 0, 0, 0, 0}, {"施加虚弱 2", 0, 1, 0, 0, 2, 0}}});

        enemySequence.push_back({
            "精英敌人：诅咒祭司", 135, true, false,
            {{"施加易伤 2 + 攻击 10", 10, 1, 0, 0, 0, 2}, {"格挡 20", 0, 1, 20, 0, 0, 0}, {"攻击 16", 16, 1, 0, 0, 0, 0}, {"施加虚弱 2 + 攻击 10", 10, 1, 0, 0, 2, 0}}});

        enemySequence.push_back({
            "BOSS：尖塔守卫", 180, false, true,
            {{"强化 +4 力量", 0, 1, 0, 4, 0, 0}, {"攻击 20", 20, 1, 0, 0, 0, 0}, {"攻击 12 x2", 12, 2, 0, 0, 0, 0}, {"格挡 22 + 施加虚弱 2", 0, 1, 22, 0, 2, 0}, {"施加易伤 2 + 攻击 15", 15, 1, 0, 0, 0, 2}}});

        floorPlan = defaultFloorPlan();
    }

    static int parseWinsFromProgressLine(const std::string& line) {
        if (line.rfind("wins=", 0) != 0) {
            return 0;
        }
        try {
            return std::max(0, std::stoi(line.substr(5)));
        } catch (...) {
            return 0;
        }
    }

    static int extractWinsFromProgressLines(const std::vector<std::string>& lines) {
        int parsedWins = 0;
        for (const auto& line : lines) {
            if (line.rfind("wins=", 0) == 0) {
                parsedWins = parseWinsFromProgressLine(line);
            }
        }
        return parsedWins;
    }

    static std::string serializeWinsProgressLine(int wins) {
        return "wins=" + std::to_string(std::max(0, wins));
    }

    void loadProgress() {
        std::ifstream in(kSavePath);
        if (!in) {
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        totalWins = extractWinsFromProgressLines(lines);
    }

    void saveProgress() const {
        std::ofstream out(kSavePath, std::ios::trunc);
        if (!out) {
            return;
        }
        out << serializeWinsProgressLine(totalWins) << "\n";
    }

    const CardDef* findCard(int id) const {
        const auto it = cardsById.find(id);
        if (it == cardsById.end()) {
            return nullptr;
        }
        return &it->second;
    }

    static bool isUpgradedCard(int id) {
        return id >= 100;
    }

    static int baseCardId(int id) {
        return isUpgradedCard(id) ? (id - 100) : id;
    }

    static int upgradedCardId(int id) {
        return isUpgradedCard(id) ? id : (id + 100);
    }

    static int calculateCardPrice(const CardDef& c) {
        int price = 40 + c.cost * 20;
        if (c.damage >= 12 || c.block >= 12) {
            price += 24;
        }
        if (c.draw >= 2) {
            price += 18;
        }
        if (c.draw >= 4) {
            price += 10;
        }
        if (c.gainEnergy > 0) {
            price += 14;
        }
        if (c.gainEnergy >= 2) {
            price += 8;
        }
        if (c.draw >= 3 && c.gainEnergy > 0) {
            price += 10;
        }
        if (c.grantsEnergyPerTurn) {
            price += 28;
            if (c.cost <= 1) {
                price += 10;
            }
        }
        if (c.redrawByExhaustingHand) {
            price += 10;
        }
        if (c.grantsStrengthPerTurn) {
            price += 22;
            if (c.cost <= 2) {
                price += 8;
            }
        }
        if (c.exhaust) {
            price -= 6;
        }
        return std::clamp(price, 35, 190);
    }

    static int battleGoldForFloor(int floorIndex) {
        const int lateBonus = std::max(0, floorIndex - 9) * 2;
        return std::clamp(35 + floorIndex * 3 + lateBonus, 35, 85);
    }

    static int battleRoomBonus(bool isElite, bool isBoss) {
        if (isBoss) {
            return 25;
        }
        if (isElite) {
            return 12;
        }
        return 0;
    }

    static bool isValidEventChoice(int eventId, int option) {
        if (option != 0 && option != 1) {
            return false;
        }
        return eventId == 1 || eventId == 2;
    }

    static int eventGoldReward(int eventId, int option) {
        if (eventId == 1 && option == 1) {
            return 180;
        }
        return 0;
    }

    static int turnStartEnergy(int energyPerTurn) {
        return Balance::kBaseTurnEnergy + std::max(0, energyPerTurn);
    }

    static bool validateFlatEnergyCurve(int energyPerTurn, int turns, int expectedEnergy) {
        const int normalizedTurns = std::max(0, turns);
        for (int t = 0; t < normalizedTurns; ++t) {
            if (turnStartEnergy(energyPerTurn) != expectedEnergy) {
                return false;
            }
        }
        return true;
    }

    static int netEnergyAfterPlay(int startEnergy, int cardCost, int gainEnergy) {
        return std::max(0, startEnergy - cardCost) + std::max(0, gainEnergy);
    }

    static std::vector<int> simulateEnergyAfterPlaySequence(int startEnergy, const std::vector<std::pair<int, int>>& costsAndGains) {
        std::vector<int> energies;
        energies.reserve(costsAndGains.size());
        int energy = std::max(0, startEnergy);
        for (const auto& entry : costsAndGains) {
            energy = netEnergyAfterPlay(energy, entry.first, entry.second);
            energies.push_back(energy);
        }
        return energies;
    }

    static bool equalsIntVector(const std::vector<int>& a, const std::vector<int>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }

    static std::vector<int> simulateTurnStartEnergyWithSourcePlays(const std::vector<int>& sourcePlaysPerTurn) {
        std::vector<int> turnStartEnergies;
        turnStartEnergies.reserve(sourcePlaysPerTurn.size());
        int energyPerTurn = 0;
        for (int plays : sourcePlaysPerTurn) {
            turnStartEnergies.push_back(turnStartEnergy(energyPerTurn));
            energyPerTurn += std::max(0, plays);
        }
        return turnStartEnergies;
    }

    static int stokeRedrawCountFromHandSize(int handSize) {
        return std::max(0, handSize - 1);
    }

    static void moveAllHandCardsToExhaust(std::vector<int>& handCards, std::vector<int>& exhaustPileCards) {
        for (int id : handCards) {
            exhaustPileCards.push_back(id);
        }
        handCards.clear();
    }

    static int simulateDrawCount(int requestCount, int drawPileCount, int discardPileCount, int currentHandSize) {
        const auto result = simulateDrawWithState(requestCount, drawPileCount, discardPileCount, currentHandSize);
        return result.drawn;
    }

    struct DrawSimResult {
        int drawn = 0;
        int remainingDraw = 0;
        int remainingDiscard = 0;
        int endingHand = 0;
    };

    static DrawSimResult simulateDrawWithState(int requestCount, int drawPileCount, int discardPileCount, int currentHandSize) {
        int drawn = 0;
        int drawCount = std::max(0, drawPileCount);
        int discardCount = std::max(0, discardPileCount);
        int handSize = std::max(0, currentHandSize);
        const int target = std::max(0, requestCount);

        for (int i = 0; i < target; ++i) {
            if (handSize >= 10) {
                break;
            }
            if (drawCount == 0) {
                if (discardCount == 0) {
                    break;
                }
                drawCount = discardCount;
                discardCount = 0;
            }
            drawCount--;
            handSize++;
            drawn++;
        }

        return {drawn, drawCount, discardCount, handSize};
    }

    static int simulateStokeDrawAfterExhaust(int handSizeWithStoke, int drawPileCount, int discardPileCount) {
        const int redraw = stokeRedrawCountFromHandSize(handSizeWithStoke);
        return simulateDrawCount(redraw, drawPileCount, discardPileCount, 0);
    }

    static std::vector<int> simulateStokeConsecutiveTurnDraws(
        const std::vector<int>& handSizesWithStoke,
        int initialDrawPile,
        int initialDiscardPile) {
        int drawPile = std::max(0, initialDrawPile);
        int discardPile = std::max(0, initialDiscardPile);
        std::vector<int> draws;
        draws.reserve(handSizesWithStoke.size());

        for (int handSizeWithStoke : handSizesWithStoke) {
            const int redraw = stokeRedrawCountFromHandSize(handSizeWithStoke);
            const DrawSimResult result = simulateDrawWithState(redraw, drawPile, discardPile, 0);
            draws.push_back(result.drawn);
            drawPile = result.remainingDraw;
            discardPile = result.remainingDiscard;
        }

        return draws;
    }

    static void applySpecificUpgradeAdjustments(int baseId, CardDef& up) {
        if (baseId == 37) {
            up.cost = 0;
            up.loseHp = 3;
            up.gainEnergy = 3;
            up.desc = "失去 3 点生命，获得 3 点能量";
        }
        if (baseId == 38) {
            up.cost = 0;
            up.gainEnergy = 2;
            up.desc = "获得 2 点能量，消耗";
        }
        if (baseId == 39) {
            up.cost = 1;
            up.desc = "每回合开始时获得 1 点能量，消耗";
        }
        if (baseId == 40) {
            up.cost = 0;
            up.desc = "消耗手牌中其余所有卡牌，然后抽取等量卡牌";
        }
    }

    static CardDef buildUpgradedCard(const CardDef& base) {
        CardDef up = base;
        up.id = 100 + base.id;
        up.name = base.name + "+";
        if (up.cost > 1) {
            up.cost -= 1;
        }
        if (up.damage > 0) {
            up.damage += 3;
        }
        if (up.block > 0) {
            up.block += 3;
        }
        if (up.draw > 0) {
            up.draw += 1;
        }
        if (up.gainStrength > 0) {
            up.gainStrength += 1;
        }
        if (up.applyWeak > 0) {
            up.applyWeak += 1;
        }
        if (up.applyVulnerable > 0) {
            up.applyVulnerable += 1;
        }
        if (up.loseHp > 0) {
            up.loseHp = std::max(0, up.loseHp - 2);
        }
        applySpecificUpgradeAdjustments(base.id, up);
        up.desc += "（升级）";
        return up;
    }

    int cardPrice(int cardId) const {
        const CardDef* c = findCard(cardId);
        if (!c) {
            return 90;
        }
        return calculateCardPrice(*c);
    }

    bool tryUpgradeDeckCard(int deckIndex) {
        if (deckIndex < 0 || deckIndex >= static_cast<int>(masterDeck.size())) {
            return false;
        }
        const int current = masterDeck[deckIndex];
        if (isUpgradedCard(current)) {
            pushLog("该卡牌已经升级");
            return false;
        }
        const int target = upgradedCardId(current);
        if (!findCard(target)) {
            pushLog("该卡牌暂无升级版本");
            return false;
        }
        masterDeck[deckIndex] = target;
        const CardDef* after = findCard(target);
        pushLog("锻造成功：" + (after ? after->name : std::string("已升级")));
        return true;
    }

    void setupShopChoices() {
        shopChoices.clear();
        if (baseCardIds.empty()) {
            return;
        }
        std::uniform_int_distribution<int> dist(0, static_cast<int>(baseCardIds.size()) - 1);
        while (shopChoices.size() < 8) {
            const int id = baseCardIds[dist(rng)];
            if (id == 1 || id == 2) {
                continue;
            }
            const CardDef* card = findCard(id);
            if (card && card->eventOnly) {
                continue;
            }
            if (std::find(shopChoices.begin(), shopChoices.end(), id) == shopChoices.end()) {
                shopChoices.push_back(id);
            }
        }
    }

    void beginDeckEdit(DeckEditMode mode, Phase returnPhase, bool advanceAfterDone) {
        if (mode == DeckEditMode::UpgradeOne && !hasUpgradableCardInDeck()) {
            pushLog("没有可升级卡牌，跳过锻造");
            showActionHint("无可升级卡牌，已跳过锻造", sf::Color(255, 210, 140));
            transitionTo(returnPhase, "跳过卡组编辑");
            if (advanceAfterDone) {
                goNextFloor();
            }
            return;
        }

        if ((mode == DeckEditMode::RemoveOne || mode == DeckEditMode::RemoveTwo) && masterDeck.empty()) {
            pushLog("牌组为空，无法移除");
            showActionHint("牌组为空，无法移除", sf::Color(255, 190, 160));
            transitionTo(returnPhase, "跳过卡组编辑");
            if (advanceAfterDone) {
                goNextFloor();
            }
            return;
        }

        deckEditMode = mode;
        deckEditRemaining = (mode == DeckEditMode::RemoveTwo) ? 2 : 1;
        deckEditReturnPhase = returnPhase;
        deckEditAdvanceAfterDone = advanceAfterDone;
        transitionTo(Phase::DeckEdit, "进入卡组编辑阶段");
    }

    void finishDeckEditIfDone() {
        if (deckEditRemaining > 0) {
            return;
        }
        showActionHint("卡组操作完成", sf::Color(160, 240, 180));
        transitionTo(deckEditReturnPhase, "卡组编辑完成");
        if (deckEditAdvanceAfterDone) {
            goNextFloor();
        }
        deckEditMode = DeckEditMode::None;
        deckEditAdvanceAfterDone = false;
    }

    bool removeDeckCardAt(int deckIndex) {
        if (deckIndex < 0 || deckIndex >= static_cast<int>(masterDeck.size())) {
            return false;
        }
        const CardDef* c = findCard(masterDeck[deckIndex]);
        pushLog("移除卡牌：" + (c ? c->name : std::string("未知")));
        masterDeck.erase(masterDeck.begin() + deckIndex);
        return true;
    }

    void tryBuyShopCard(int slot) {
        if (slot < 0 || slot >= static_cast<int>(shopChoices.size())) {
            return;
        }
        const int id = shopChoices[slot];
        if (id <= 0) {
            pushLog("该商品已售出");
            showActionHint("该商品已售出", sf::Color(200, 200, 200));
            return;
        }
        const int price = cardPrice(id);
        if (gold < price) {
            pushLog("金币不足，无法购买");
            showActionHint("金币不足", sf::Color(255, 170, 170));
            return;
        }
        gold -= price;
        masterDeck.push_back(id);
        const CardDef* c = findCard(id);
        pushLog("购买卡牌：" + (c ? c->name : std::string("未知")) + "（-" + std::to_string(price) + " 金币）");
        showActionHint("购买成功：" + (c ? c->name : std::string("卡牌")), sf::Color(170, 255, 190));
        shopChoices[slot] = -1;
    }

    void tryBuyPotionCapacity() {
        if (potionPurchasedThisRun) {
            pushLog("本场游戏已购买过药水");
            showActionHint("本场游戏限购1瓶", sf::Color(255, 190, 160));
            return;
        }
        if (gold < potionCapacityPrice) {
            pushLog("金币不足，无法购买药水");
            showActionHint("金币不足", sf::Color(255, 170, 170));
            return;
        }
        gold -= potionCapacityPrice;
        potionCount = std::min(potionCount + 1, potionMax);
        potionPurchasedThisRun = true;
        pushLog("购买药水（-" + std::to_string(potionCapacityPrice) + " 金币）");
        showActionHint("获得1瓶药水", sf::Color(180, 220, 255));
    }

    void tryBuyRemoveService() {
        if (masterDeck.empty()) {
            pushLog("牌组为空，无法移除");
            showActionHint("牌组为空", sf::Color(255, 190, 160));
            return;
        }
        if (gold < deckRemovePrice) {
            pushLog("金币不足，无法购买移除服务");
            showActionHint("金币不足", sf::Color(255, 170, 170));
            return;
        }
        gold -= deckRemovePrice;
        pushLog("开始移除卡牌（-" + std::to_string(deckRemovePrice) + " 金币）");
        showActionHint("选择1张卡牌移除", sf::Color(240, 220, 180));
        deckRemovePrice += 30;
        beginDeckEdit(DeckEditMode::RemoveOne, Phase::Shop, false);
    }

    void resolveEventChoice(int option) {
        if (!isValidEventChoice(currentEventId, option)) {
            pushLog("无效事件选项");
            showActionHint("无效选项", sf::Color(255, 180, 180));
            return;
        }

        if (currentEventId == 1) {
            if (option == 0) {
                player.hp = std::max(0, player.hp - 16);
                masterDeck.push_back(36);
                pushLog("你失去 16 点生命，并获得 邪恶之刃");
                if (player.hp <= 0) {
                    transitionTo(Phase::Defeat, "事件中生命归零");
                    return;
                }
                showActionHint("失去16生命，获得邪恶之刃", sf::Color(255, 210, 170));
                goNextFloor();
                return;
            }
            const int gainedGold = eventGoldReward(currentEventId, option);
            gold += gainedGold;
            pushLog("你获得 " + std::to_string(gainedGold) + " 金币");
            showActionHint("获得" + std::to_string(gainedGold) + "金币", sf::Color(200, 255, 160));
            goNextFloor();
            return;
        }

        if (currentEventId == 2) {
            if (option == 0) {
                std::vector<RelicType> candidates = {
                    RelicType::EmberCharm,
                    RelicType::IronSigil,
                    RelicType::BloodVial,
                    RelicType::TacticianLens,
                    RelicType::ThornEmblem
                };
                candidates.erase(
                    std::remove_if(candidates.begin(), candidates.end(), [&](RelicType relic) { return hasRelic(relic); }),
                    candidates.end());
                if (!candidates.empty()) {
                    std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);
                    grantRelic(candidates[dist(rng)]);
                } else {
                    gold += 50;
                    pushLog("你获得 50 金币（遗物已满）");
                    showActionHint("获得50金币（遗物已满）", sf::Color(255, 226, 150));
                }
                goNextFloor();
                return;
            }
            beginDeckEdit(DeckEditMode::RemoveTwo, Phase::Event, true);
        }
    }

    void enterCurrentFloor() {
        if (currentFloor < 0 || currentFloor >= static_cast<int>(floorPlan.size())) {
            transitionTo(Phase::VictoryThanks, "楼层完成后进入感谢页");
            phaseTimer = 0.0f;
            return;
        }

        campfireDidAction = false;
        const FloorNode& node = floorPlan[currentFloor];
        switch (node.type) {
            case RoomType::Battle:
                currentEnemyIndex = std::clamp(node.enemyIndex, 0, static_cast<int>(enemySequence.size()) - 1);
                transitionTo(Phase::Battle, "进入战斗房间");
                startBattle();
                break;
            case RoomType::Campfire:
                transitionTo(Phase::Campfire, "进入营火房间");
                break;
            case RoomType::Shop:
                setupShopChoices();
                transitionTo(Phase::Shop, "进入商店房间");
                break;
            case RoomType::Event:
                currentEventId = node.eventId;
                transitionTo(Phase::Event, "进入事件房间");
                break;
        }
    }

    void goNextFloor() {
        currentFloor++;
        enterCurrentFloor();
    }

    static Intent fallbackIntent() {
        return {"待机", 0, 1, 0, 0, 0, 0};
    }

    void resetRun() {
        player.maxHp = Balance::kStartingMaxHp;
        player.hp = Balance::kStartingMaxHp;
        player.block = 0;
        player.strength = 0;
        player.weak = 0;
        player.vulnerable = 0;

        playerStrengthPerTurn = 0;
        playerEnergyPerTurn = 0;
        gold = Balance::kStartingGold;
        potionCount = Balance::kStartingPotionCount;
        potionMax = Balance::kStartingPotionMax;
        potionCapacityPrice = Balance::kStartingPotionCapacityPrice;
        deckRemovePrice = Balance::kStartingDeckRemovePrice;
        eliteRelicDropChance = Balance::kEliteRelicDropChance;
        enemyHitFlashTimer = 0.0f;
        playerHitFlashTimer = 0.0f;
        turnBannerTimer = 0.0f;
        hoveredCardIndex = -1;

        drawPile.clear();
        discardPile.clear();
        hand.clear();
        exhaustPile.clear();
        rewardChoices.clear();
        logs.clear();
        masterDeck.clear();
        floatingTexts.clear();
        pendingCard.active = false;
        pendingEnemy.active = false;
        deckEditMode = DeckEditMode::None;
        deckEditRemaining = 0;
        deckEditReturnPhase = Phase::MainMenu;
        deckEditAdvanceAfterDone = false;
        shopChoices.clear();
        currentEventId = 0;
        actionHint.clear();
        actionHintTimer = 0.0f;
        showOwnedDeckOverlay = false;
        showRelicOverlay = false;
        activePileOverlay = PileOverlayType::None;
        battleQuoteText.clear();
        battleQuoteTimer = 0.0f;
        battleTurnCount = 0;
        relics.clear();

        masterDeck = starterDeckTemplate();

        currentFloor = 0;
        phase = Phase::MainMenu;
        phaseTimer = 0.0f;
        enterCurrentFloor();
    }

    void beginCardPlay(int handIndex) {
        if (pendingCard.active || pendingEnemy.active || handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
            return;
        }
        if (!enableCombatAnimations) {
            playSfx(sfxCardPlayBuf, 40.f);
            applyCard(handIndex);
            return;
        }
        auto rects = handRects();
        if (handIndex >= static_cast<int>(rects.size())) {
            return;
        }
        pendingCard.active = true;
        pendingCard.handIndex = handIndex;
        pendingCard.cardId = hand[handIndex];
        pendingCard.elapsed = 0.0f;
        pendingCard.duration = 0.16f;
        pendingCard.fromRect = rects[handIndex];
        pendingCard.target = sf::Vector2f(620.f, 300.f);
    }

    void beginEnemyTurn() {
        if (phase != Phase::Battle || pendingEnemy.active || pendingCard.active) {
            return;
        }
        if (!enableCombatAnimations) {
            executeEnemyTurn(true);
            return;
        }
        pendingEnemy.active = true;
        pendingEnemy.resolved = false;
        pendingEnemy.elapsed = 0.0f;
        pendingEnemy.windup = 0.22f;
        pendingEnemy.recover = 0.12f;
        spawnFloatingText("敌人行动", sf::Vector2f(620.f, 186.f), sf::Color(255, 210, 120), 0.5f);
    }

    void shuffle(std::vector<int>& pile) { std::shuffle(pile.begin(), pile.end(), rng); }

    void drawCards(int amount) {
        for (int i = 0; i < amount; ++i) {
            if (hand.size() >= 10) {
                return;
            }
            if (drawPile.empty()) {
                if (discardPile.empty()) {
                    return;
                }
                drawPile = discardPile;
                discardPile.clear();
                shuffle(drawPile);
            }
            hand.push_back(drawPile.back());
            drawPile.pop_back();
        }
    }

    /**
     * @brief 开始战斗
     * 
     * 初始化战斗状态：
     * 1. 重置牌堆（抽牌堆=主牌组，洗牌）
     * 2. 清空手牌、弃牌堆、消耗堆
     * 3. 重置玩家战斗状态（格挡、力量、debuff）
     * 4. 初始化敌人（根据楼层调整难度）
     * 5. 触发遗物效果
     * 
     * 敌人难度调整：
     * - Boss血量 x1.6
     * - 第6层起每层HP+5
     */
    void startBattle() {
        if (currentEnemyIndex < 0 || currentEnemyIndex >= static_cast<int>(enemySequence.size())) {
            phase = Phase::MainMenu;
            return;
        }

        drawPile = masterDeck;
        shuffle(drawPile);
        hand.clear();
        discardPile.clear();
        exhaustPile.clear();
        pendingCard.active = false;
        pendingEnemy.active = false;
        showOwnedDeckOverlay = false;
        showRelicOverlay = false;
        activePileOverlay = PileOverlayType::None;
        battleQuoteText.clear();
        battleQuoteTimer = 0.0f;
        battleTurnCount = 0;

        player.block = 0;
        player.strength = 0;
        player.weak = 0;
        player.vulnerable = 0;
        playerStrengthPerTurn = 0;
        playerEnergyPerTurn = 0;
        rampageDamageBonus = 0;
        blockPersists = false;
        darkEmbraceActive = false;

        enemyIntentIndex = 0;

        const EnemyDef& def = enemySequence[currentEnemyIndex];
        int baseHp = def.maxHp;
        if (def.boss) {
            baseHp = static_cast<int>(baseHp * 1.6f);
        }
        if (currentFloor >= 5) {
            const int floorBonus = (currentFloor - 4) * 5;
            baseHp += floorBonus;
        }
        enemy.maxHp = baseHp;
        enemy.hp = baseHp;
        enemy.block = 0;
        enemy.strength = 0;
        enemy.weak = 0;
        enemy.vulnerable = 0;

        if (hasRelic(RelicType::BloodVial)) {
            const int oldHp = player.hp;
            player.hp = std::min(player.maxHp, player.hp + relicBattleStartHeal(true));
            const int healed = player.hp - oldHp;
            if (healed > 0) {
                pushLog("血瓶生效：战前回复 " + std::to_string(healed) + " 点生命");
            }
        }

        pushLog("战斗开始：" + def.name);
        battleQuoteText = enemyEntranceQuote(currentEnemyIndex);
        battleQuoteTimer = 4.0f;
        pushLog(battleQuoteText);
        startPlayerTurn();
    }

    void startPlayerTurn() {
        if (!blockPersists) {
            player.block = 0;
        }
        playerEnergy = turnStartEnergy(playerEnergyPerTurn);
        const int energyBonus = relicTurnStartEnergyBonus(battleTurnCount, hasRelic(RelicType::EmberCharm));
        if (energyBonus > 0) {
            playerEnergy += energyBonus;
            pushLog("余烬护符生效：首回合额外获得 1 点能量");
        }
        player.strength += playerStrengthPerTurn;
        int drawAmount = 5 + relicTurnStartDrawBonus(hasRelic(RelicType::TacticianLens));
        drawCards(drawAmount);
        battleTurnCount++;
        turnBannerTimer = 1.1f;
        pushLog("你的回合开始");
    }

    int adjustedDamage(int base, const Combatant& attacker, const Combatant& defender) const {
        int dmg = std::max(0, base + attacker.strength);
        if (attacker.weak > 0) {
            dmg = static_cast<int>(std::floor(dmg * 0.75f));
        }
        if (defender.vulnerable > 0) {
            dmg = static_cast<int>(std::floor(dmg * 1.5f));
        }
        return std::max(0, dmg);
    }

    int applyCardResourceEffects(const CardDef& card) {
        if (card.gainEnergy > 0) {
            playerEnergy += card.gainEnergy;
            pushLog(card.name + " 使你获得 " + std::to_string(card.gainEnergy) + " 点能量");
        }

        if (card.loseHp > 0) {
            player.hp = std::max(0, player.hp - card.loseHp);
            pushLog(card.name + " 使你失去 " + std::to_string(card.loseHp) + " 点生命");
            if (tearActive) {
                player.strength += 1;
                pushLog("撕裂触发：获得1点力量");
                spawnFloatingText("+1 力量", sf::Vector2f(290.f, 108.f), sf::Color(255, 200, 100), 0.8f);
            }
        }

        if (card.draw > 0) {
            drawCards(card.draw);
            pushLog(card.name + " 让你抽取 " + std::to_string(card.draw) + " 张牌");
        }

        if (card.redrawByExhaustingHand) {
            return stokeRedrawCountFromHandSize(static_cast<int>(hand.size()));
        }

        return 0;
    }

    /**
     * @brief 应用卡牌的持续成长效果
     * @param card 卡牌定义
     * 
     * 处理能力牌的持续效果：
     * - grantsStrengthPerTurn: 每回合开始获得力量
     * - grantsEnergyPerTurn: 每回合开始获得能量
     */
    void applyCardPersistentGrowthEffects(const CardDef& card) {
        if (card.grantsStrengthPerTurn) {
            playerStrengthPerTurn += 1;
            pushLog("能力生效：每回合开始获得 1 点力量");
        }

        if (card.grantsEnergyPerTurn) {
            playerEnergyPerTurn += 1;
            pushLog("能力生效：每回合开始获得 1 点能量");
        }
    }

    /**
     * @brief 造成伤害的核心函数
     * @param target 目标（玩家或敌人）
     * @param damage 伤害值（已计算加成）
     * @param targetIsPlayer 目标是否为玩家
     * 
     * 伤害计算流程：
     * 1. 先扣除格挡（格挡不保留到下回合）
     * 2. 剩余伤害扣除生命
     * 3. 触发视觉反馈（闪烁、浮动文字）
     * 4. 播放音效
     * 
     * @note 伤害值应该已经过adjustedDamage计算
     */
    void dealDamage(Combatant& target, int damage, bool targetIsPlayer) {
        if (damage <= 0) {
            return;
        }
        const int blocked = std::min(target.block, damage);
        target.block -= blocked;
        target.hp -= (damage - blocked);
        if (target.hp < 0) {
            target.hp = 0;
        }

        if (targetIsPlayer) {
            playerHitFlashTimer = 0.22f;
            spawnFloatingText("-" + std::to_string(damage), sf::Vector2f(275.f, 122.f), sf::Color(255, 120, 120), 0.65f);
            playSfx(sfxPlayerHitBuf, 50.f);
        } else {
            enemyHitFlashTimer = 0.22f;
            spawnFloatingText("-" + std::to_string(damage), sf::Vector2f(620.f, 238.f), sf::Color(255, 210, 120), 0.65f);
            playSfx(sfxSwordBuf, 50.f);
        }
    }

    /**
     * @brief 打出卡牌的核心函数
     * @param handIndex 手牌索引
     * 
     * 执行流程：
     * 1. 验证卡牌有效性
     * 2. 检查能量是否足够
     * 3. 执行卡牌效果（伤害、格挡、状态等）
     * 4. 处理特殊效果（消耗、抽牌等）
     * 5. 移动手牌到弃牌堆或消耗堆
     * 
     * @note 能量检查在调用前应该已经完成
     */
    void applyCard(int handIndex) {
        if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
            return;
        }

        const int cardId = hand[handIndex];
        const CardDef* c = findCard(cardId);
        if (!c) {
            pushLog("发现无效卡牌，已跳过");
            hand.erase(hand.begin() + handIndex);
            return;
        }

        if (playerEnergy < c->cost) {
            pushLog("能量不足，无法使用：" + c->name);
            return;
        }

        playerEnergy -= c->cost;
        int totalDamageDealt = 0;

        if (baseCardId(cardId) == 16) {
            player.strength *= 2;
        }

        if (baseCardId(cardId) == 17) {
            rampageDamageBonus += 8;
            pushLog("暴走伤害递增 +" + std::to_string(rampageDamageBonus));
        }

        if (baseCardId(cardId) == 19) {
            blockPersists = true;
            pushLog("壁垒生效：格挡将在回合结束后保留");
        }

        if (baseCardId(cardId) == 29) {
            const Intent intent = currentEnemyIntent();
            if (intent.damage > 0) {
                drawCards(3);
                pushLog("战斗专注：敌人攻击意图触发，抽3张牌");
            }
        }

        if (baseCardId(cardId) == 41) {
            int exhaustedCount = 0;
            int blockGain = 0;
            for (int i = static_cast<int>(hand.size()) - 1; i >= 0; --i) {
                const CardDef* handCard = findCard(hand[i]);
                if (handCard && handCard->type != CardType::Attack) {
                    exhaustPile.push_back(hand[i]);
                    hand.erase(hand.begin() + i);
                    exhaustedCount++;
                    blockGain += 7;
                }
            }
            if (blockGain > 0) {
                player.block += blockGain;
                pushLog("重振精神：消耗 " + std::to_string(exhaustedCount) + " 张非攻击牌，获得 " + std::to_string(blockGain) + " 点格挡");
                spawnFloatingText("+" + std::to_string(blockGain) + " 格挡", sf::Vector2f(260.f, 142.f), sf::Color(120, 210, 255), 0.8f);
            }
        }

        if (baseCardId(cardId) == 42) {
            int exhaustedCount = 0;
            int totalDmg = 0;
            for (int i = static_cast<int>(hand.size()) - 1; i >= 0; --i) {
                const CardDef* handCard = findCard(hand[i]);
                if (handCard && handCard->type == CardType::Attack) {
                    exhaustPile.push_back(hand[i]);
                    hand.erase(hand.begin() + i);
                    exhaustedCount++;
                    totalDmg += 7;
                }
            }
            if (totalDmg > 0 && enemy.hp > 0) {
                const int dmg = adjustedDamage(totalDmg, player, enemy);
                dealDamage(enemy, dmg, false);
                pushLog("战鼓：消耗 " + std::to_string(exhaustedCount) + " 张攻击牌，造成 " + std::to_string(dmg) + " 点伤害");
            }
        }

        if (baseCardId(cardId) == 43) {
            darkEmbraceActive = true;
            pushLog("黑暗之拥生效：每次消耗卡牌时抽一张牌");
        }

        if (baseCardId(cardId) == 45) {
            int blockDmg = player.block;
            if (blockDmg > 0 && enemy.hp > 0) {
                const int dmg = adjustedDamage(blockDmg, player, enemy);
                dealDamage(enemy, dmg, false);
                totalDamageDealt += dmg;
                pushLog("全身撞击造成 " + std::to_string(dmg) + " 点伤害");
            }
        }

        if (baseCardId(cardId) == 46) {
            player.block *= 2;
            pushLog("巩固：格挡翻倍至 " + std::to_string(player.block));
            spawnFloatingText("格挡x2", sf::Vector2f(260.f, 142.f), sf::Color(120, 210, 255), 0.8f);
        }

        if (baseCardId(cardId) == 48) {
            tearActive = true;
            pushLog("撕裂生效：每次失去生命时获得1点力量");
        }

        applyCardPersistentGrowthEffects(*c);

        if (c->damage > 0 && enemy.hp > 0) {
            for (int i = 0; i < c->hits; ++i) {
                int baseDmg = c->damage;
                if (baseCardId(cardId) == 17) {
                    baseDmg += rampageDamageBonus;
                }
                const int dmg = adjustedDamage(baseDmg, player, enemy);
                dealDamage(enemy, dmg, false);
                totalDamageDealt += dmg;
                pushLog(c->name + " 造成 " + std::to_string(dmg) + " 点伤害");
                if (enemy.hp <= 0) {
                    break;
                }
            }
        }

        if (c->healFromDamage && totalDamageDealt > 0) {
            const int oldHp = player.hp;
            player.hp = std::min(player.maxHp, player.hp + totalDamageDealt);
            const int healed = player.hp - oldHp;
            if (healed > 0) {
                pushLog(c->name + " 回复 " + std::to_string(healed) + " 点生命");
                spawnFloatingText("+" + std::to_string(healed) + " 生命", sf::Vector2f(260.f, 98.f), sf::Color(120, 255, 140), 0.9f);
                playSfx(sfxHealBuf, 40.f);
            }
        }

        if (c->block > 0) {
            player.block += c->block;
            pushLog(c->name + " 提供 " + std::to_string(c->block) + " 点格挡");
            spawnFloatingText("+" + std::to_string(c->block) + " 格挡", sf::Vector2f(260.f, 142.f), sf::Color(120, 210, 255), 0.8f);
            if (c->block > 0) playSfx(sfxBlockBuf, 40.f);
        }

        if (c->gainStrength > 0) {
            player.strength += c->gainStrength;
            pushLog(c->name + " 提供 " + std::to_string(c->gainStrength) + " 点力量");
            spawnFloatingText("+" + std::to_string(c->gainStrength) + " 力量", sf::Vector2f(290.f, 108.f), sf::Color(255, 200, 100), 0.8f);
        }

        if (c->applyVulnerable > 0 && enemy.hp > 0) {
            enemy.vulnerable += c->applyVulnerable;
            pushLog("敌人获得 " + std::to_string(c->applyVulnerable) + " 层易伤");
            spawnFloatingText("易伤 +" + std::to_string(c->applyVulnerable), sf::Vector2f(640.f, 208.f), sf::Color(255, 170, 90), 0.9f);
        }

        if (c->applyWeak > 0 && enemy.hp > 0) {
            enemy.weak += c->applyWeak;
            pushLog("敌人获得 " + std::to_string(c->applyWeak) + " 层虚弱");
            spawnFloatingText("虚弱 +" + std::to_string(c->applyWeak), sf::Vector2f(640.f, 238.f), sf::Color(180, 150, 255), 0.9f);
        }

        const int redrawCount = applyCardResourceEffects(*c);

        hand.erase(hand.begin() + handIndex);
        if (c->exhaust) {
            exhaustPile.push_back(cardId);
            if (darkEmbraceActive) {
                drawCards(1);
                pushLog("黑暗之拥触发：抽1张牌");
            }
        } else {
            discardPile.push_back(cardId);
        }

        if (redrawCount > 0) {
            moveAllHandCardsToExhaust(hand, exhaustPile);
            drawCards(redrawCount);
            pushLog(c->name + " 消耗了 " + std::to_string(redrawCount) + " 张手牌并抽取等量卡牌");
        }

        if (player.hp <= 0) {
            transitionTo(Phase::Defeat, "玩家因卡牌副作用死亡");
            return;
        }

        if (enemy.hp <= 0) {
            onBattleWon();
        }
    }

    void onBattleWon() {
        pushLog("战斗胜利");
        potionCount = std::min(potionMax, potionCount + 1);
        const EnemyDef* def = nullptr;
        if (currentEnemyIndex >= 0 && currentEnemyIndex < static_cast<int>(enemySequence.size())) {
            def = &enemySequence[currentEnemyIndex];
        }
        const int battleGold = battleGoldForFloor(currentFloor) + battleRoomBonus(def && def->elite, def && def->boss);
        gold += battleGold;
        pushLog("获得 " + std::to_string(battleGold) + " 金币与 1 瓶药剂（当前金币 " + std::to_string(gold) + "）");
        showActionHint("战斗奖励：+" + std::to_string(battleGold) + " 金币", sf::Color(200, 255, 170));
        if (def != nullptr && def->elite) {
            if (tryGrantEliteRelicDrop()) {
                pushLog("精英掉落了遗物");
            } else {
                pushLog("本次精英未掉落遗物");
            }
        }
        if (currentFloor == static_cast<int>(floorPlan.size()) - 1) {
            pushLog("尖塔守卫倒下：余烬仍在，你已穿越黑暗。");
            totalWins++;
            saveProgress();
            transitionTo(Phase::VictoryThanks, "击败最终BOSS");
            phaseTimer = 0.0f;
            return;
        }

        rewardChoices.clear();
        std::uniform_int_distribution<int> dist(0, static_cast<int>(cardPool.size()) - 1);
        const bool isElite = def && def->elite;
        const int rewardCount = isElite ? 6 : 3;
        while (rewardChoices.size() < rewardCount) {
            int id = cardPool[dist(rng)].id;
            if (id == 1 || id == 2) {
                continue;
            }
            const CardDef* card = findCard(id);
            if (card && card->eventOnly) {
                continue;
            }
            if (std::find(rewardChoices.begin(), rewardChoices.end(), id) == rewardChoices.end()) {
                rewardChoices.push_back(id);
            }
        }

        bool hasLowCost = false;
        for (int id : rewardChoices) {
            const CardDef* reward = findCard(id);
            if (reward && reward->cost <= 1) {
                hasLowCost = true;
                break;
            }
        }

        if (!hasLowCost) {
            std::vector<int> lowCostCandidates;
            for (const auto& c : cardPool) {
                if (c.id == 1 || c.id == 2 || c.cost > 1) {
                    continue;
                }
                if (std::find(rewardChoices.begin(), rewardChoices.end(), c.id) == rewardChoices.end()) {
                    lowCostCandidates.push_back(c.id);
                }
            }
            if (!lowCostCandidates.empty()) {
                std::uniform_int_distribution<int> slotDist(0, static_cast<int>(rewardChoices.size()) - 1);
                std::uniform_int_distribution<int> cardDist(0, static_cast<int>(lowCostCandidates.size()) - 1);
                rewardChoices[slotDist(rng)] = lowCostCandidates[cardDist(rng)];
            }
        }
        transitionTo(Phase::Reward, "普通战斗胜利后进入奖励");
    }

    void endPlayerTurn() {
        for (int id : hand) {
            discardPile.push_back(id);
        }
        hand.clear();

        if (player.weak > 0) {
            --player.weak;
        }
        if (player.vulnerable > 0) {
            --player.vulnerable;
        }

        beginEnemyTurn();
    }

    void executeEnemyTurn(bool startNextPlayerTurn) {
        if (phase != Phase::Battle) {
            return;
        }

        enemy.block = 0;

        const EnemyDef& def = enemySequence[currentEnemyIndex];
        if (def.pattern.empty()) {
            pushLog("敌人行为模式为空，跳过敌方回合");
            if (startNextPlayerTurn) {
                startPlayerTurn();
            }
            return;
        }

        const Intent& intent = def.pattern[enemyIntentIndex % def.pattern.size()];
        enemyIntentIndex++;

        if (intent.block > 0) {
            enemy.block += intent.block;
            pushLog("敌人获得 " + std::to_string(intent.block) + " 点格挡");
        }

        if (intent.gainStrength > 0) {
            enemy.strength += intent.gainStrength;
            pushLog("敌人获得 " + std::to_string(intent.gainStrength) + " 点力量");
            spawnFloatingText("敌人力量 +" + std::to_string(intent.gainStrength), sf::Vector2f(620.f, 208.f), sf::Color(255, 180, 110), 0.9f);
        }

        if (intent.applyWeak > 0) {
            player.weak += intent.applyWeak;
            pushLog("你获得 " + std::to_string(intent.applyWeak) + " 层虚弱");
            spawnFloatingText("虚弱 +" + std::to_string(intent.applyWeak), sf::Vector2f(272.f, 154.f), sf::Color(180, 150, 255), 0.9f);
        }

        if (intent.applyVulnerable > 0) {
            player.vulnerable += intent.applyVulnerable;
            pushLog("你获得 " + std::to_string(intent.applyVulnerable) + " 层易伤");
            spawnFloatingText("易伤 +" + std::to_string(intent.applyVulnerable), sf::Vector2f(272.f, 182.f), sf::Color(255, 170, 90), 0.9f);
        }

        if (intent.damage > 0) {
            int floorDamageBonus = 0;
            if (currentFloor >= 5) {
                floorDamageBonus = (currentFloor - 4);
            }
            for (int i = 0; i < intent.hits; ++i) {
                const int dmg = adjustedDamage(intent.damage + floorDamageBonus, enemy, player);
                dealDamage(player, dmg, true);
                pushLog("敌人造成 " + std::to_string(dmg) + " 点伤害");
                if (hasRelic(RelicType::ThornEmblem) && enemy.hp > 0) {
                    const int thorns = relicThornDamage(true);
                    dealDamage(enemy, thorns, false);
                    pushLog("荆棘徽记反伤 " + std::to_string(thorns) + " 点伤害");
                    if (enemy.hp <= 0) {
                        onBattleWon();
                        return;
                    }
                }
                if (player.hp <= 0) {
                    break;
                }
            }
        }

        if (enemy.weak > 0) {
            --enemy.weak;
        }
        if (enemy.vulnerable > 0) {
            --enemy.vulnerable;
        }

        if (player.hp <= 0) {
            transitionTo(Phase::Defeat, "敌方回合造成玩家死亡");
            return;
        }

        if (startNextPlayerTurn) {
            startPlayerTurn();
        }
    }

    std::vector<sf::FloatRect> handRects() const {
        std::vector<sf::FloatRect> rects;
        const int count = static_cast<int>(hand.size());
        float cardW = 94.f;
        float cardH = 188.f;
        float gap = 6.f;
        float y = 488.f;

        if (count >= 8) {
            cardW = 88.f;
            cardH = 180.f;
            gap = 4.f;
            y = 496.f;
        }
        if (count >= 10) {
            cardW = 80.f;
            cardH = 170.f;
            gap = 2.f;
            y = 506.f;
        }
        if (count >= 12) {
            cardW = 72.f;
            cardH = 162.f;
            gap = 1.f;
            y = 514.f;
        }

        const float maxHandWidth = 1180.f;
        float totalW = static_cast<float>(count) * cardW +
                       static_cast<float>(std::max(0, count - 1)) * gap;
        if (count > 1 && totalW > maxHandWidth) {
            gap = std::max(0.f, (maxHandWidth - static_cast<float>(count) * cardW) / static_cast<float>(count - 1));
            totalW = static_cast<float>(count) * cardW + static_cast<float>(count - 1) * gap;
        }

        float startX = (1280.f - totalW) * 0.5f;
        startX = std::max(10.f, startX);
        for (size_t i = 0; i < hand.size(); ++i) {
            rects.emplace_back(startX + i * (cardW + gap), y, cardW, cardH);
        }
        return rects;
    }

    std::vector<sf::FloatRect> rewardRects() const {
        std::vector<sf::FloatRect> rects;
        const float cardW = 220.f;
        const float cardH = 280.f;
        const float gap = 40.f;
        const float totalW = 3.f * cardW + 2.f * gap;
        float startX = (1280.f - totalW) * 0.5f;
        const float y = 220.f;
        for (int i = 0; i < 3; ++i) {
            rects.emplace_back(startX + i * (cardW + gap), y, cardW, cardH);
        }
        rects.emplace_back(440.f, 540.f, 160.f, 50.f);
        rects.emplace_back(680.f, 540.f, 160.f, 50.f);
        return rects;
    }

    std::vector<sf::FloatRect> shopCardRects() const {
        std::vector<sf::FloatRect> rects;
        const float cardW = 190.f;
        const float cardH = 150.f;
        const float gapX = 24.f;
        const float gapY = 20.f;
        const float startX = 80.f;
        const float startY = 210.f;
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 4; ++c) {
                rects.emplace_back(startX + c * (cardW + gapX), startY + r * (cardH + gapY), cardW, cardH);
            }
        }
        return rects;
    }

    std::vector<sf::FloatRect> deckEditRects() const {
        std::vector<sf::FloatRect> rects;
        const float cardW = 150.f;
        const float cardH = 86.f;
        const float gapX = 12.f;
        const float gapY = 10.f;
        const float startX = 40.f;
        const float startY = 140.f;
        const int cols = 7;
        for (size_t i = 0; i < masterDeck.size(); ++i) {
            const int row = static_cast<int>(i) / cols;
            const int col = static_cast<int>(i) % cols;
            rects.emplace_back(startX + col * (cardW + gapX), startY + row * (cardH + gapY), cardW, cardH);
        }
        return rects;
    }

    const Intent& currentEnemyIntent() const {
        static const Intent fallback = fallbackIntent();
        if (currentEnemyIndex < 0 || currentEnemyIndex >= static_cast<int>(enemySequence.size())) {
            return fallback;
        }
        const EnemyDef& def = enemySequence[currentEnemyIndex];
        if (def.pattern.empty()) {
            return fallback;
        }
        return def.pattern[enemyIntentIndex % def.pattern.size()];
    }

    /**
     * @brief 选择奖励卡牌
     * @param index 选择的奖励索引
     * 
     * 奖励系统逻辑：
     * - index 4: 查看牌组（不消耗奖励）
     * - index 3 或 7: 跳过奖励
     * - 其他: 选择对应卡牌加入牌组
     * 
     * 精英奖励特殊处理：
     * - 精英敌人可以选择多张卡牌
     * - 选择后减少剩余选择次数
     */
    void pickReward(int index) {
        if (index == 4) {
            activePileOverlay = PileOverlayType::Deck;
            return;
        }
        if (index == 3 || (rewardChoices.size() > 3 && index == 7)) {
            pushLog("跳过卡牌奖励");
            eliteRewardPicksRemaining = 0;
            goNextFloor();
            return;
        }
        if (index < 0 || index >= static_cast<int>(rewardChoices.size())) {
            return;
        }
        const int picked = rewardChoices[index];
        masterDeck.push_back(picked);
        const CardDef* pickedCard = findCard(picked);
        if (pickedCard) {
            pushLog("获得卡牌：" + pickedCard->name);
        } else {
            pushLog("获得卡牌：未知卡牌");
        }
        rewardChoices.erase(rewardChoices.begin() + index);

        const EnemyDef* def = nullptr;
        if (currentEnemyIndex >= 0 && currentEnemyIndex < static_cast<int>(enemySequence.size())) {
            def = &enemySequence[currentEnemyIndex];
        }
        const bool isElite = def && def->elite;
        if (isElite && eliteRewardPicksRemaining > 0) {
            eliteRewardPicksRemaining--;
            pushLog("精英奖励：还需选择 " + std::to_string(eliteRewardPicksRemaining) + " 张卡牌");
            return;
        }

        goNextFloor();
    }

    /**
     * @brief 事件处理函数
     * 
     * 处理所有SFML窗口事件（键盘、鼠标）
     * 
     * 键盘快捷键：
     * - R: 重置当前运行（战斗/奖励/战败阶段）
     * - T: 切换动画效果
     * - 任意键: 在胜利感谢页进入制作人名单
     * 
     * 鼠标事件：
     * - 左键点击： 处理各阶段UI交互
     * 
     * @note 事件处理顺序很重要，先处理全局快捷键，     */
    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
            if (phase == Phase::Battle || phase == Phase::Reward || phase == Phase::Defeat) {
                resetRun();
            }
            return;
        }

        if (event.type == sf::Event::KeyPressed) {
            if (phase == Phase::VictoryThanks) {
                transitionTo(Phase::Credits, "按键进入制作人名单");
                phaseTimer = 0.0f;
                return;
            }
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::T) {
            enableCombatAnimations = !enableCombatAnimations;
            pushLog(std::string("动画效果：") + (enableCombatAnimations ? "开启" : "关闭"));
            if (!enableCombatAnimations) {
                if (pendingCard.active) {
                    const int toResolve = pendingCard.handIndex;
                    pendingCard.active = false;
                    applyCard(toResolve);
                }
                if (pendingEnemy.active) {
                    if (!pendingEnemy.resolved) {
                        pendingEnemy.resolved = true;
                        executeEnemyTurn(false);
                    }
                    pendingEnemy.active = false;
                    if (phase == Phase::Battle && player.hp > 0) {
                        startPlayerTurn();
                    }
                }
            }
            return;
        }

        if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) {
            return;
        }

        const sf::Vector2f mouse(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

        if (bgmToggleRect.contains(mouse)) {
            playSfx(sfxUiClickBuf, 50.f);
            toggleBgm();
            return;
        }

        if (phase == Phase::MainMenu) {
            if (menuStartRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                resetRun();
                return;
            }
            if (menuRulesRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                phase = Phase::Rules;
                return;
            }
            if (menuExitRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                window.close();
                return;
            }
        } else if (phase == Phase::Rules) {
            if (backMenuRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                phase = Phase::MainMenu;
                return;
            }
        } else if (phase == Phase::Battle) {
            if (pendingCard.active || pendingEnemy.active) {
                return;
            }

            if (devTestRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                dealDamage(enemy, 999, false);
                pushLog("开发者测试：造成999点伤害");
                if (enemy.hp <= 0) {
                    onBattleWon();
                }
                return;
            }

            if (showOwnedDeckOverlay) {
                if (runStatsRect.contains(mouse) || ownedDeckCloseRect.contains(mouse)) {
                    showOwnedDeckOverlay = false;
                }
                return;
            }

            if (showRelicOverlay) {
                if (relicInspectRect.contains(mouse) || relicOverlayCloseRect.contains(mouse)) {
                    showRelicOverlay = false;
                }
                return;
            }

            if (relicInspectRect.contains(mouse)) {
                activePileOverlay = PileOverlayType::None;
                showOwnedDeckOverlay = false;
                showRelicOverlay = true;
                return;
            }

            if (runStatsRect.contains(mouse)) {
                activePileOverlay = PileOverlayType::None;
                showRelicOverlay = false;
                showOwnedDeckOverlay = true;
                return;
            }

            if (activePileOverlay != PileOverlayType::None) {
                if (pileOverlayCloseRect.contains(mouse)) {
                    activePileOverlay = PileOverlayType::None;
                    return;
                }
                if (drawPileRect.contains(mouse)) {
                    activePileOverlay = PileOverlayType::Draw;
                    return;
                }
                if (discardPileRect.contains(mouse)) {
                    activePileOverlay = PileOverlayType::Discard;
                    return;
                }
                if (exhaustPileRect.contains(mouse)) {
                    activePileOverlay = PileOverlayType::Exhaust;
                    return;
                }
                return;
            }

            if (drawPileRect.contains(mouse)) {
                showOwnedDeckOverlay = false;
                activePileOverlay = PileOverlayType::Draw;
                return;
            }
            if (discardPileRect.contains(mouse)) {
                showOwnedDeckOverlay = false;
                activePileOverlay = PileOverlayType::Discard;
                return;
            }
            if (exhaustPileRect.contains(mouse)) {
                showOwnedDeckOverlay = false;
                activePileOverlay = PileOverlayType::Exhaust;
                return;
            }

            if (potionRect.contains(mouse)) {
                if (potionCount > 0) {
                    potionCount--;
                    player.block += 10;
                    drawCards(1);
                    pushLog("使用药剂：获得 10 点格挡并抽 1 张牌");
                    spawnFloatingText("药剂：+10 格挡", sf::Vector2f(1080.f, 506.f), sf::Color(200, 180, 255), 0.9f);
                } else {
                    pushLog("药剂已用完");
                }
                return;
            }

            if (endTurnRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                endPlayerTurn();
                return;
            }

            auto rects = handRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    playSfx(sfxUiClickBuf, 40.f);
                    beginCardPlay(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Reward) {
            if (activePileOverlay != PileOverlayType::None) {
                if (pileOverlayCloseRect.contains(mouse)) {
                    activePileOverlay = PileOverlayType::None;
                    return;
                }
                return;
            }
            auto rects = rewardRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    playSfx(sfxUiClickBuf, 50.f);
                    pickReward(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Campfire) {
            if (campRestRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                if (campfireDidAction) {
                    pushLog("本层营火已使用");
                    showActionHint("本层营火已使用", sf::Color(255, 210, 170));
                    return;
                }
                const int oldHp = player.hp;
                player.hp = std::min(player.maxHp, player.hp + 30);
                pushLog("营火休息：回复 " + std::to_string(player.hp - oldHp) + " 点生命");
                showActionHint("休息回复 " + std::to_string(player.hp - oldHp) + " 生命", sf::Color(170, 255, 190));
                campfireDidAction = true;
                goNextFloor();
                return;
            }
            if (campForgeRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                if (campfireDidAction) {
                    pushLog("本层营火已使用");
                    showActionHint("本层营火已使用", sf::Color(255, 210, 170));
                    return;
                }
                campfireDidAction = true;
                beginDeckEdit(DeckEditMode::UpgradeOne, Phase::Campfire, true);
                return;
            }
        } else if (phase == Phase::Shop) {
            if (shopLeaveRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                goNextFloor();
                return;
            }
            if (shopPotionRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                tryBuyPotionCapacity();
                return;
            }
            if (shopRemoveRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                tryBuyRemoveService();
                return;
            }

            auto rects = shopCardRects();
            for (size_t i = 0; i < rects.size() && i < shopChoices.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    playSfx(sfxUiClickBuf, 50.f);
                    tryBuyShopCard(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Event) {
            if (eventLeftRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                resolveEventChoice(0);
                return;
            }
            if (eventRightRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                resolveEventChoice(1);
                return;
            }
        } else if (phase == Phase::DeckEdit) {
            if (deckDoneRect.contains(mouse)) {
                playSfx(sfxUiClickBuf, 50.f);
                if (deckEditRemaining > 0) {
                    pushLog("还需要选择 " + std::to_string(deckEditRemaining) + " 张卡牌");
                    showActionHint("仍需选择 " + std::to_string(deckEditRemaining) + " 张", sf::Color(255, 220, 160));
                } else {
                    transitionTo(deckEditReturnPhase, "手动结束卡组编辑");
                    if (deckEditAdvanceAfterDone) {
                        goNextFloor();
                    }
                    deckEditMode = DeckEditMode::None;
                    deckEditAdvanceAfterDone = false;
                    showActionHint("卡组操作完成", sf::Color(160, 240, 180));
                }
                return;
            }

            auto rects = deckEditRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (!rects[i].contains(mouse)) {
                    continue;
                }

                bool changed = false;
                if (deckEditMode == DeckEditMode::UpgradeOne) {
                    changed = tryUpgradeDeckCard(static_cast<int>(i));
                } else if (deckEditMode == DeckEditMode::RemoveOne || deckEditMode == DeckEditMode::RemoveTwo) {
                    changed = removeDeckCardAt(static_cast<int>(i));
                }

                if (changed) {
                    deckEditRemaining = std::max(0, deckEditRemaining - 1);
                    finishDeckEditIfDone();
                }
                return;
            }
        } else if (phase == Phase::VictoryThanks) {
            transitionTo(Phase::Credits, "点击跳过感谢页");
            phaseTimer = 0.0f;
        } else if (phase == Phase::Credits || phase == Phase::Defeat) {
            if (backMenuRect.contains(mouse)) {
                transitionTo(Phase::MainMenu, "结算页返回主菜单");
                phaseTimer = 0.0f;
            }
        }
    }

    void update(float dt) {
        enemyHitFlashTimer = std::max(0.0f, enemyHitFlashTimer - dt);
        playerHitFlashTimer = std::max(0.0f, playerHitFlashTimer - dt);
        turnBannerTimer = std::max(0.0f, turnBannerTimer - dt);
        actionHintTimer = std::max(0.0f, actionHintTimer - dt);
        battleQuoteTimer = std::max(0.0f, battleQuoteTimer - dt);
        sfxCooldown = std::max(0.0f, sfxCooldown - dt);

        for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
            it->remaining = std::max(0.0f, it->remaining - dt);
            it->position += it->velocity * dt;
            if (it->remaining <= 0.0f) {
                it = floatingTexts.erase(it);
            } else {
                ++it;
            }
        }

        if (pendingCard.active) {
            pendingCard.elapsed += dt;
            if (pendingCard.elapsed >= pendingCard.duration) {
                const int toResolve = pendingCard.handIndex;
                pendingCard.active = false;
                playSfx(sfxCardPlayBuf, 40.f);
                applyCard(toResolve);
            }
        }

        if (pendingEnemy.active) {
            pendingEnemy.elapsed += dt;

            if (!pendingEnemy.resolved && pendingEnemy.elapsed >= pendingEnemy.windup) {
                pendingEnemy.resolved = true;
                pendingEnemy.elapsed = 0.0f;
                executeEnemyTurn(false);
                if (phase != Phase::Battle || player.hp <= 0) {
                    pendingEnemy.active = false;
                }
            } else if (pendingEnemy.resolved && pendingEnemy.elapsed >= pendingEnemy.recover) {
                pendingEnemy.active = false;
                if (phase == Phase::Battle && player.hp > 0) {
                    startPlayerTurn();
                }
            }
        }

        hoveredCardIndex = -1;
        if (phase == Phase::Battle) {
            const sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            const sf::Vector2f mousePos(static_cast<float>(pixelPos.x), static_cast<float>(pixelPos.y));
            auto rects = handRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mousePos)) {
                    hoveredCardIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        if (phase == Phase::VictoryThanks) {
        }
    }

    void drawSfText(const sf::String& text, float x, float y, unsigned int size, sf::Color color) {
        if (!hasFont) {
            return;
        }
        sf::Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setPosition(x, y);
        window.draw(t);
    }

    void drawTextCentered(const std::string& text, float y, unsigned int size, sf::Color color) {
        if (!hasFont) {
            return;
        }
        sf::Text t;
        t.setFont(font);
        const sf::String utfText = sf::String::fromUtf8(text.begin(), text.end());
        t.setString(utfText);
        t.setCharacterSize(size);
        t.setFillColor(color);
        const float textWidth = t.getLocalBounds().width;
        const float x = (1280.f - textWidth) / 2.f;
        t.setPosition(x, y);
        window.draw(t);
    }

    void drawText(const std::string& text, float x, float y, unsigned int size, sf::Color color) {
        const sf::String utfText = sf::String::fromUtf8(text.begin(), text.end());
        drawSfText(utfText, x, y, size, color);
    }

    void drawWrappedText(const std::string& text, float x, float y, unsigned int size, sf::Color color, int maxCharsPerLine) {
        if (maxCharsPerLine <= 0) {
            drawText(text, x, y, size, color);
            return;
        }

        const sf::String utfText = sf::String::fromUtf8(text.begin(), text.end());

        sf::String line;
        int charCount = 0;
        float offsetY = 0.0f;
        for (std::size_t i = 0; i < utfText.getSize(); ++i) {
            const sf::Uint32 codepoint = utfText[i];

            if (codepoint == '\n') {
                drawSfText(line, x, y + offsetY, size, color);
                line.clear();
                charCount = 0;
                offsetY += static_cast<float>(size) * 1.25f;
                continue;
            }

            line += codepoint;
            charCount++;
            if (charCount >= maxCharsPerLine) {
                drawSfText(line, x, y + offsetY, size, color);
                line.clear();
                charCount = 0;
                offsetY += static_cast<float>(size) * 1.25f;
            }
        }

        if (line.getSize() > 0) {
            drawSfText(line, x, y + offsetY, size, color);
        }
    }

    void drawCenteredWrappedText(const std::string& text, float y, unsigned int size, sf::Color color, int maxCharsPerLine) {
        if (!hasFont) {
            return;
        }
        if (maxCharsPerLine <= 0) {
            sf::Text t;
            t.setFont(font);
            t.setString(sf::String::fromUtf8(text.begin(), text.end()));
            t.setCharacterSize(size);
            const sf::FloatRect bounds = t.getLocalBounds();
            const float x = (1280.f - bounds.width) * 0.5f;
            t.setPosition(x, y);
            t.setFillColor(color);
            window.draw(t);
            return;
        }

        const sf::String utfText = sf::String::fromUtf8(text.begin(), text.end());

        sf::String line;
        int charCount = 0;
        float offsetY = 0.0f;
        for (std::size_t i = 0; i < utfText.getSize(); ++i) {
            const sf::Uint32 codepoint = utfText[i];

            if (codepoint == '\n') {
                sf::Text t;
                t.setFont(font);
                t.setString(line);
                t.setCharacterSize(size);
                const sf::FloatRect bounds = t.getLocalBounds();
                const float x = (1280.f - bounds.width) * 0.5f;
                t.setPosition(x, y + offsetY);
                t.setFillColor(color);
                window.draw(t);
                line.clear();
                charCount = 0;
                offsetY += static_cast<float>(size) * 1.25f;
                continue;
            }

            line += codepoint;
            charCount++;
            if (charCount >= maxCharsPerLine) {
                sf::Text t;
                t.setFont(font);
                t.setString(line);
                t.setCharacterSize(size);
                const sf::FloatRect bounds = t.getLocalBounds();
                const float x = (1280.f - bounds.width) * 0.5f;
                t.setPosition(x, y + offsetY);
                t.setFillColor(color);
                window.draw(t);
                line.clear();
                charCount = 0;
                offsetY += static_cast<float>(size) * 1.25f;
            }
        }

        if (line.getSize() > 0) {
            sf::Text t;
            t.setFont(font);
            t.setString(line);
            t.setCharacterSize(size);
            const sf::FloatRect bounds = t.getLocalBounds();
            const float x = (1280.f - bounds.width) * 0.5f;
            t.setPosition(x, y + offsetY);
            t.setFillColor(color);
            window.draw(t);
        }
    }

    int wrappedLineCount(const std::string& text, int maxCharsPerLine) const {
        if (text.empty()) {
            return 1;
        }
        if (maxCharsPerLine <= 0) {
            return 1;
        }

        const sf::String utfText = sf::String::fromUtf8(text.begin(), text.end());
        int lines = 1;
        int charCount = 0;
        for (std::size_t i = 0; i < utfText.getSize(); ++i) {
            const sf::Uint32 codepoint = utfText[i];
            if (codepoint == '\n') {
                lines++;
                charCount = 0;
                continue;
            }

            charCount++;
            if (charCount >= maxCharsPerLine && i + 1 < utfText.getSize()) {
                lines++;
                charCount = 0;
            }
        }
        return std::max(1, lines);
    }

    static bool isTextureSizeAvailable(const sf::Vector2u& size) {
        return size.x > 0u && size.y > 0u;
    }

    void drawTextureFit(const sf::Texture& tex, const sf::FloatRect& rect, sf::Color tint = sf::Color::White) {
        if (!isTextureSizeAvailable(tex.getSize())) {
            sf::RectangleShape placeholder(sf::Vector2f(rect.width, rect.height));
            placeholder.setPosition(rect.left, rect.top);
            placeholder.setFillColor(sf::Color(90, 36, 36, 210));
            placeholder.setOutlineThickness(2.f);
            placeholder.setOutlineColor(sf::Color(255, 180, 180, 230));
            window.draw(placeholder);
            drawText("资源缺失", rect.left + 8.f, rect.top + rect.height * 0.4f, 16, sf::Color(255, 230, 210));
            return;
        }
        sf::Sprite sp(tex);
        const float sx = rect.width / static_cast<float>(tex.getSize().x);
        const float sy = rect.height / static_cast<float>(tex.getSize().y);
        sp.setScale(sx, sy);
        sp.setPosition(rect.left, rect.top);
        sp.setColor(tint);
        window.draw(sp);
    }

    const sf::Texture* cardIcon(CardType type) const {
        if (type == CardType::Attack) {
            return &cardAttackTex;
        }
        if (type == CardType::Skill) {
            return &cardSkillTex;
        }
        return &cardPowerTex;
    }

    const sf::Texture* enemyPortrait() const {
        if (currentEnemyIndex < 0 || currentEnemyIndex >= static_cast<int>(enemySequence.size())) {
            return &enemy1Tex;
        }
        switch (currentEnemyIndex) {
            case 0: return &enemy1Tex; // 狡猾狐狸
            case 1: return &enemy2Tex; // 邪教徒
            case 2: return &enemy3Tex; // 剧毒曼巴
            case 3: return &enemy4Tex; // 刀盾卫兵
            case 4: return &elite1Tex; // 饿狼首领
            case 5: return &elite2Tex; // 鲜血斗士
            case 6: return &elite3Tex; // 诅咒祭司
            case 7: return &bossTex;   // 尖塔守卫
            default: return &enemy1Tex;
        }
    }

    sf::FloatRect enemyPortraitRect() const {
        if (currentEnemyIndex >= 7) {
            return sf::FloatRect(504.f, 154.f, 272.f, 272.f);
        }
        if (currentEnemyIndex >= 4) {
            return sf::FloatRect(516.f, 166.f, 248.f, 248.f);
        }
        return sf::FloatRect(530.f, 178.f, 220.f, 220.f);
    }

    sf::Color cardColor(CardType type) const {
        switch (type) {
            case CardType::Attack: return sf::Color(190, 80, 80);
            case CardType::Skill: return sf::Color(80, 130, 190);
            case CardType::Power: return sf::Color(190, 160, 70);
            default: return sf::Color(140, 140, 140);
        }
    }

    sf::Color cardColorWithUpgrade(CardType type, int cardId) const {
        if (isUpgradedCard(cardId)) {
            switch (type) {
                case CardType::Attack: return sf::Color(60, 170, 80);
                case CardType::Skill: return sf::Color(50, 160, 100);
                case CardType::Power: return sf::Color(80, 180, 90);
                default: return sf::Color(60, 160, 70);
            }
        }
        return cardColor(type);
    }

    sf::Color cardTitleColorWithUpgrade(int cardId) const {
        if (isUpgradedCard(cardId)) {
            return sf::Color(255, 255, 140);
        }
        return sf::Color(255, 255, 230);
    }

    sf::Color cardMetaColorWithUpgrade(int cardId) const {
        if (isUpgradedCard(cardId)) {
            return sf::Color(210, 255, 200);
        }
        return sf::Color(230, 230, 230);
    }

    void drawBackground(const sf::Texture& tex) {
        if (isTextureSizeAvailable(tex.getSize())) {
            drawTextureFit(tex, sf::FloatRect(0.f, 0.f, 1280.f, 720.f));
        }
    }

    void drawButton(const sf::FloatRect& rect, const std::string& label, const sf::Color& bg) {
        sf::RectangleShape box(sf::Vector2f(rect.width, rect.height));
        box.setPosition(rect.left, rect.top);
        box.setFillColor(bg);
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(238, 221, 170, 180));
        window.draw(box);
        if (hasFont) {
            sf::Text t;
            t.setFont(font);
            t.setString(sf::String::fromUtf8(label.begin(), label.end()));
            t.setCharacterSize(30);
            t.setFillColor(sf::Color(250, 248, 240));
            const sf::FloatRect bounds = t.getLocalBounds();
            const float x = rect.left + (rect.width - bounds.width) * 0.5f - bounds.left;
            const float y = rect.top + (rect.height - bounds.height) * 0.5f - bounds.top - 3.f;
            t.setPosition(x, y);
            window.draw(t);
        } else {
            drawText(label, rect.left + 110.f, rect.top + 22.f, 30, sf::Color::White);
        }
    }

    void renderActionHint() {
        if (actionHintTimer <= 0.0f || actionHint.empty()) {
            return;
        }
        const sf::Uint8 alpha = static_cast<sf::Uint8>(110.f + std::min(1.0f, actionHintTimer / 2.3f) * 120.f);
        sf::RectangleShape box(sf::Vector2f(640.f, 42.f));
        box.setPosition(320.f, 676.f);
        box.setFillColor(sf::Color(20, 24, 32, alpha));
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(0, 0, 0, alpha));
        window.draw(box);

        sf::Color c = actionHintColor;
        c.a = alpha;
        drawText(actionHint, 340.f, 686.f, 22, c);
    }

    void renderStatusPanel() {
        sf::RectangleShape topBar(sf::Vector2f(1280.f, 124.f));
        topBar.setPosition(0.f, 0.f);
        topBar.setFillColor(sf::Color(18, 34, 42, 235));
        window.draw(topBar);

        sf::RectangleShape divider(sf::Vector2f(1280.f, 2.f));
        divider.setPosition(0.f, 122.f);
        divider.setFillColor(sf::Color(246, 224, 162, 130));
        window.draw(divider);

        drawTextureFit(logoTex, sf::FloatRect(18.f, 14.f, 36.f, 36.f));
        drawText("尖塔战士", 62.f, 11.f, 30, sf::Color::White);
        drawText("第 " + std::to_string(currentFloor + 1) + " / " + std::to_string(floorPlan.size()) + " 层", 560.f, 18.f, 24, sf::Color(236, 228, 188));

        sf::RectangleShape playerPanel(sf::Vector2f(430.f, 64.f));
        playerPanel.setPosition(20.f, 52.f);
        playerPanel.setFillColor(sf::Color(26, 55, 62, 235));
        playerPanel.setOutlineThickness(2.f);
        playerPanel.setOutlineColor(sf::Color(225, 239, 216, 150));
        window.draw(playerPanel);

        std::ostringstream p;
        p << "玩家 " << player.hp << "/" << player.maxHp << "  格挡 " << player.block << "  力量 " << player.strength
          << "  虚弱 " << player.weak << "  易伤 " << player.vulnerable;
        drawTextureFit(statHpTex, sf::FloatRect(32.f, 62.f, 16.f, 16.f));
        drawText(p.str(), 52.f, 60.f, 20, sf::Color(210, 238, 210));
        drawTextureFit(statEnergyTex, sf::FloatRect(32.f, 88.f, 16.f, 16.f));
        drawText("能量 " + std::to_string(playerEnergy), 52.f, 86.f, 24, sf::Color(255, 236, 160));

        sf::RectangleShape runStats(sf::Vector2f(runStatsRect.width, runStatsRect.height));
        runStats.setPosition(runStatsRect.left, runStatsRect.top);
        runStats.setFillColor(sf::Color(48, 58, 34, 238));
        runStats.setOutlineThickness(2.f);
        runStats.setOutlineColor(sf::Color(255, 230, 166, 228));
        window.draw(runStats);

        auto drawStatBadge = [&](sf::Texture& tex, const sf::FloatRect& iconRect) {
            sf::RectangleShape badge(sf::Vector2f(iconRect.width + 6.f, iconRect.height + 6.f));
            badge.setPosition(iconRect.left - 3.f, iconRect.top - 3.f);
            badge.setFillColor(sf::Color(76, 65, 42, 186));
            badge.setOutlineThickness(1.f);
            badge.setOutlineColor(sf::Color(248, 228, 170, 148));
            window.draw(badge);
            drawTextureFit(tex, iconRect);
        };

        drawStatBadge(uiGoldTex, sf::FloatRect(runStatsRect.left + 12.f, runStatsRect.top + 6.f, 19.f, 19.f));
        drawText("金币 " + std::to_string(gold), runStatsRect.left + 38.f, runStatsRect.top + 7.f, 18, sf::Color(255, 238, 184));
        drawStatBadge(uiPotionTex, sf::FloatRect(runStatsRect.left + 176.f, runStatsRect.top + 6.f, 19.f, 19.f));
        drawText("药剂 " + std::to_string(potionCount) + "/" + std::to_string(potionMax), runStatsRect.left + 202.f, runStatsRect.top + 7.f, 18, sf::Color(244, 236, 216));

        drawStatBadge(uiDeckTex, sf::FloatRect(runStatsRect.left + 12.f, runStatsRect.top + 32.f, 19.f, 19.f));
        drawText("卡组 " + std::to_string(masterDeck.size()), runStatsRect.left + 38.f, runStatsRect.top + 33.f, 18, sf::Color(232, 232, 212));
        drawStatBadge(uiRelicTex, sf::FloatRect(runStatsRect.left + 176.f, runStatsRect.top + 32.f, 19.f, 19.f));
        drawText("遗物 " + std::to_string(relics.size()), runStatsRect.left + 202.f, runStatsRect.top + 33.f, 18, sf::Color(232, 232, 212));

        drawText("遗物：" + relicSummaryShort(), runStatsRect.left + 12.f, runStatsRect.top + 56.f, 15, sf::Color(232, 214, 170));
        drawText("点击查看已拥有全部卡牌", runStatsRect.left + 12.f, runStatsRect.top + 76.f, 15, sf::Color(215, 205, 170));
        sf::RectangleShape relicBtn(sf::Vector2f(relicInspectRect.width, relicInspectRect.height));
        relicBtn.setPosition(relicInspectRect.left, relicInspectRect.top);
        relicBtn.setFillColor(sf::Color(104, 92, 50, 230));
        relicBtn.setOutlineThickness(1.f);
        relicBtn.setOutlineColor(sf::Color(250, 234, 182, 178));
        window.draw(relicBtn);
        drawText("查看遗物总览", relicInspectRect.left + 10.f, relicInspectRect.top + 1.f, 16, sf::Color(255, 238, 182));

        if (playerHitFlashTimer > 0.0f) {
            const float ratio = std::min(1.0f, playerHitFlashTimer / 0.22f);
            sf::RectangleShape flash(sf::Vector2f(1280.f, 124.f));
            flash.setPosition(0.f, 0.f);
            flash.setFillColor(sf::Color(220, 40, 40, static_cast<sf::Uint8>(35.f + ratio * 80.f)));
            window.draw(flash);
        }
    }

    void renderOwnedDeckOverlay() {
        sf::RectangleShape curtain(sf::Vector2f(1280.f, 720.f));
        curtain.setFillColor(sf::Color(8, 10, 18, 200));
        window.draw(curtain);

        sf::RectangleShape panel(sf::Vector2f(980.f, 560.f));
        panel.setPosition(150.f, 90.f);
        panel.setFillColor(sf::Color(24, 30, 44, 245));
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(sf::Color(240, 216, 148, 220));
        window.draw(panel);

        drawText("当前持有卡牌总览", 182.f, 112.f, 34, sf::Color(250, 238, 180));

        sf::RectangleShape closeBtn(sf::Vector2f(ownedDeckCloseRect.width, ownedDeckCloseRect.height));
        closeBtn.setPosition(ownedDeckCloseRect.left, ownedDeckCloseRect.top);
        closeBtn.setFillColor(sf::Color(90, 56, 56, 230));
        closeBtn.setOutlineThickness(2.f);
        closeBtn.setOutlineColor(sf::Color(0, 0, 0, 180));
        window.draw(closeBtn);
        drawText("关闭总览", ownedDeckCloseRect.left + 42.f, ownedDeckCloseRect.top + 9.f, 22, sf::Color::White);

        std::unordered_map<int, int> counts;
        for (int id : masterDeck) {
            counts[id] += 1;
        }

        std::vector<std::pair<int, int>> entries;
        entries.reserve(counts.size());
        for (const auto& item : counts) {
            entries.push_back(item);
        }
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        float y = 172.f;
        int shown = 0;
        for (const auto& item : entries) {
            const CardDef* c = findCard(item.first);
            if (!c) {
                continue;
            }
            drawText(c->name + " x" + std::to_string(item.second), 182.f, y, 22, sf::Color(230, 230, 230));
            drawText("费 " + std::to_string(c->cost) + "  " + c->desc, 430.f, y + 2.f, 18, sf::Color(190, 206, 220));
            y += 30.f;
            shown++;
            if (y > 612.f) {
                break;
            }
        }

        if (shown == 0) {
            drawText("当前没有卡牌", 182.f, 174.f, 24, sf::Color(255, 190, 190));
        }

        drawText("再次点击右上角面板或" + std::string("关闭总览") + "可返回战斗", 182.f, 618.f, 20, sf::Color(206, 206, 206));
    }

    void renderRelicOverlay() {
        sf::RectangleShape curtain(sf::Vector2f(1280.f, 720.f));
        curtain.setFillColor(sf::Color(8, 10, 18, 204));
        window.draw(curtain);

        sf::RectangleShape panel(sf::Vector2f(980.f, 560.f));
        panel.setPosition(150.f, 90.f);
        panel.setFillColor(sf::Color(30, 28, 22, 246));
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(sf::Color(255, 220, 150, 220));
        window.draw(panel);

        drawText("当前持有遗物总览", 182.f, 112.f, 34, sf::Color(250, 238, 180));

        sf::RectangleShape closeBtn(sf::Vector2f(relicOverlayCloseRect.width, relicOverlayCloseRect.height));
        closeBtn.setPosition(relicOverlayCloseRect.left, relicOverlayCloseRect.top);
        closeBtn.setFillColor(sf::Color(90, 56, 56, 230));
        closeBtn.setOutlineThickness(2.f);
        closeBtn.setOutlineColor(sf::Color(0, 0, 0, 180));
        window.draw(closeBtn);
        drawText("关闭遗物总览", relicOverlayCloseRect.left + 24.f, relicOverlayCloseRect.top + 9.f, 22, sf::Color::White);

        if (relics.empty()) {
            drawText("当前尚未获得遗物", 182.f, 176.f, 24, sf::Color(255, 198, 198));
            return;
        }

        float y = 172.f;
        for (size_t i = 0; i < relics.size(); ++i) {
            drawText(std::to_string(static_cast<int>(i) + 1) + ". " + relicName(relics[i]), 182.f, y, 24, sf::Color(240, 232, 212));
            drawText(relicDesc(relics[i]), 420.f, y + 2.f, 20, sf::Color(218, 206, 180));
            y += 38.f;
            if (y > 616.f) {
                break;
            }
        }
    }

    void renderPileButtons() {
        auto drawPileButton = [&](const sf::FloatRect& rect, const std::string& label, int count, const sf::Color& color, bool active) {
            sf::RectangleShape btn(sf::Vector2f(rect.width, rect.height));
            btn.setPosition(rect.left, rect.top);
            btn.setFillColor(active ? sf::Color(color.r, color.g, color.b, 255) : sf::Color(color.r, color.g, color.b, 220));
            btn.setOutlineThickness(2.f);
            btn.setOutlineColor(active ? sf::Color(255, 244, 188) : sf::Color(18, 18, 18, 200));
            window.draw(btn);
            drawText(label, rect.left + 9.f, rect.top + 7.f, 18, sf::Color::White);
            drawText("数量 " + std::to_string(count), rect.left + 9.f, rect.top + 31.f, 16, sf::Color(230, 230, 220));
        };

        drawPileButton(drawPileRect, "抽牌堆", static_cast<int>(drawPile.size()), sf::Color(56, 94, 126), activePileOverlay == PileOverlayType::Draw);
        drawPileButton(discardPileRect, "弃牌堆", static_cast<int>(discardPile.size()), sf::Color(106, 84, 60), activePileOverlay == PileOverlayType::Discard);
        drawPileButton(exhaustPileRect, "消耗堆", static_cast<int>(exhaustPile.size()), sf::Color(104, 62, 78), activePileOverlay == PileOverlayType::Exhaust);
    }

    void renderPileOverlay() {
        if (activePileOverlay == PileOverlayType::None) {
            return;
        }

        const std::vector<int>* source = nullptr;
        std::string title;
        if (activePileOverlay == PileOverlayType::Draw) {
            source = &drawPile;
            title = "抽牌堆";
        } else if (activePileOverlay == PileOverlayType::Discard) {
            source = &discardPile;
            title = "弃牌堆";
        } else if (activePileOverlay == PileOverlayType::Exhaust) {
            source = &exhaustPile;
            title = "消耗堆";
        } else {
            source = &masterDeck;
            title = "牌组";
        }

        sf::RectangleShape curtain(sf::Vector2f(1280.f, 720.f));
        curtain.setFillColor(sf::Color(9, 11, 18, 180));
        window.draw(curtain);

        sf::RectangleShape panel(sf::Vector2f(860.f, 520.f));
        panel.setPosition(210.f, 100.f);
        panel.setFillColor(sf::Color(26, 34, 46, 245));
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(sf::Color(255, 224, 156, 210));
        window.draw(panel);

        drawText(title + "总览", 238.f, 122.f, 34, sf::Color(252, 240, 188));
        drawText("顶端在上方，点击左下按钮可切换堆", 238.f, 160.f, 18, sf::Color(210, 210, 210));

        sf::RectangleShape closeBtn(sf::Vector2f(pileOverlayCloseRect.width, pileOverlayCloseRect.height));
        closeBtn.setPosition(pileOverlayCloseRect.left, pileOverlayCloseRect.top);
        closeBtn.setFillColor(sf::Color(88, 54, 54, 230));
        closeBtn.setOutlineThickness(2.f);
        closeBtn.setOutlineColor(sf::Color(0, 0, 0, 180));
        window.draw(closeBtn);
        drawText("关闭牌堆总览", pileOverlayCloseRect.left + 24.f, pileOverlayCloseRect.top + 9.f, 22, sf::Color::White);

        if (source == nullptr || source->empty()) {
            drawText("当前牌堆为空", 238.f, 210.f, 26, sf::Color(255, 192, 192));
            return;
        }

        float y = 206.f;
        int shown = 0;
        for (auto it = source->rbegin(); it != source->rend(); ++it) {
            const CardDef* c = findCard(*it);
            if (!c) {
                continue;
            }
            drawText(std::to_string(shown + 1) + ". " + c->name, 238.f, y, 22, sf::Color(236, 236, 236));
            drawText("费 " + std::to_string(c->cost) + "  " + c->desc, 430.f, y + 2.f, 18, sf::Color(192, 208, 224));
            y += 30.f;
            shown++;
            if (y > 584.f) {
                break;
            }
        }

        if (shown < static_cast<int>(source->size())) {
            drawText("已显示前 " + std::to_string(shown) + " 张，共 " + std::to_string(source->size()) + " 张", 238.f, 588.f, 18, sf::Color(206, 206, 206));
        }
    }

    /**
     * @brief 渲染战斗场景
     * 
     * 战斗界面布局：
     * - 左侧：玩家状态面板（HP、能量、格挡、力量）
     * - 中间：敌人立绘和信息面板
     * - 右侧：手牌区域
     * - 底部：抽牌堆、弃牌堆按钮
     * - 右下角：结束回合按钮
     * 
     * 动画效果：
     * - 敌人受击闪烁（红色）
     * - 玩家受击闪烁（红色）
     * - 浮动文字（伤害/治疗数值）
     */
    void renderBattle() {
        renderStatusPanel();
        renderPileButtons();

        sf::RectangleShape enemyInfoPanel(sf::Vector2f(320.f, 120.f));
        enemyInfoPanel.setPosition(920.f, 128.f);
        enemyInfoPanel.setFillColor(sf::Color(53, 43, 33, 230));
        enemyInfoPanel.setOutlineThickness(2.f);
        enemyInfoPanel.setOutlineColor(sf::Color(255, 226, 150, 220));
        window.draw(enemyInfoPanel);

        if (currentEnemyIndex >= 0 && currentEnemyIndex < static_cast<int>(enemySequence.size())) {
            const EnemyDef& def = enemySequence[currentEnemyIndex];
            drawText(def.name, 932.f, 136.f, 19, sf::Color(255, 224, 172));
        }

        std::ostringstream e;
        e << "生命 " << enemy.hp << "/" << enemy.maxHp << "  格挡 " << enemy.block;
        drawText(e.str(), 932.f, 162.f, 17, sf::Color(236, 208, 208));
        drawText("力量 " + std::to_string(enemy.strength) + "  虚弱 " + std::to_string(enemy.weak) + "  易伤 " + std::to_string(enemy.vulnerable),
             932.f, 184.f, 17, sf::Color(232, 200, 200));

        const Intent& intent = currentEnemyIntent();
        drawTextureFit(intentTex, sf::FloatRect(932.f, 208.f, 22.f, 22.f));
        drawText("意图：" + intent.label, 962.f, 207.f, 16, sf::Color(255, 235, 150));

        sf::RectangleShape enemyPanel(sf::Vector2f(440.f, 280.f));
        enemyPanel.setPosition(420.f, 146.f);
        enemyPanel.setFillColor(sf::Color(28, 46, 55, 220));
        enemyPanel.setOutlineThickness(2.f);
        enemyPanel.setOutlineColor(sf::Color(198, 223, 228, 140));
        window.draw(enemyPanel);
        const sf::FloatRect portraitRect = enemyPortraitRect();
        drawTextureFit(*enemyPortrait(), portraitRect);

        if (pendingEnemy.active && !pendingEnemy.resolved) {
            sf::RectangleShape windup(sf::Vector2f(198.f, 36.f));
            windup.setPosition(538.f, 146.f);
            windup.setFillColor(sf::Color(150, 90, 40, 210));
            windup.setOutlineThickness(2.f);
            windup.setOutlineColor(sf::Color(0, 0, 0, 180));
            window.draw(windup);
            drawText("敌人蓄力中...", 560.f, 153.f, 18, sf::Color::White);
        }

        if (battleQuoteTimer > 0.0f && !battleQuoteText.empty()) {
            sf::RectangleShape quoteBox(sf::Vector2f(300.f, 56.f));
            quoteBox.setPosition(836.f, 258.f);
            quoteBox.setFillColor(sf::Color(42, 34, 28, 228));
            quoteBox.setOutlineThickness(2.f);
            quoteBox.setOutlineColor(sf::Color(255, 211, 140, 210));
            window.draw(quoteBox);
            drawWrappedText(battleQuoteText, 848.f, 270.f, 15, sf::Color(255, 236, 188), 16);
        }

        if (enemyHitFlashTimer > 0.0f) {
            const float ratio = std::min(1.0f, enemyHitFlashTimer / 0.22f);
            sf::RectangleShape enemyFlash(sf::Vector2f(440.f, 280.f));
            enemyFlash.setPosition(420.f, 146.f);
            enemyFlash.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(30.f + ratio * 120.f)));
            window.draw(enemyFlash);
        }

        sf::RectangleShape endTurn(sf::Vector2f(endTurnRect.width, endTurnRect.height));
        endTurn.setPosition(endTurnRect.left, endTurnRect.top);
        endTurn.setFillColor(sf::Color(54, 146, 92));
        endTurn.setOutlineThickness(2.f);
        endTurn.setOutlineColor(sf::Color(232, 248, 218, 130));
        window.draw(endTurn);
        drawText("结束回合", endTurnRect.left + 8.f, endTurnRect.top + 10.f, 18, sf::Color::White);

        sf::RectangleShape potionBtn(sf::Vector2f(potionRect.width, potionRect.height));
        potionBtn.setPosition(potionRect.left, potionRect.top);
        potionBtn.setFillColor(potionCount > 0 ? sf::Color(98, 76, 138, 220) : sf::Color(76, 76, 76, 190));
        potionBtn.setOutlineThickness(2.f);
        potionBtn.setOutlineColor(sf::Color(15, 15, 15, 190));
        window.draw(potionBtn);
        drawText("药剂 " + std::to_string(potionCount) + "/" + std::to_string(potionMax), potionRect.left + 7.f, potionRect.top + 8.f, 13, sf::Color(245, 235, 180));
        drawText("+10格挡 抽1", potionRect.left + 7.f, potionRect.top + 28.f, 12, sf::Color(220, 220, 220));

        auto rects = handRects();
        for (size_t i = 0; i < hand.size(); ++i) {
            if (pendingCard.active && static_cast<int>(i) == pendingCard.handIndex) {
                continue;
            }
            const CardDef* c = findCard(hand[i]);
            if (!c) {
                continue;
            }

            const float hoverLift = (static_cast<int>(i) == hoveredCardIndex) ? 12.f : 0.f;
            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top - hoverLift);
            card.setFillColor(cardColorWithUpgrade(c->type, hand[i]));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c->name, rects[i].left + 6.f, rects[i].top + 6.f - hoverLift, 18, cardTitleColorWithUpgrade(hand[i]));
            drawText("费 " + std::to_string(c->cost), rects[i].left + 6.f, rects[i].top + 32.f - hoverLift, 14, cardMetaColorWithUpgrade(hand[i]));
            drawTextureFit(*cardIcon(c->type), sf::FloatRect(rects[i].left + 56.f, rects[i].top + 6.f - hoverLift, 30.f, 30.f));
            drawWrappedText(c->desc, rects[i].left + 6.f, rects[i].top + 55.f - hoverLift, 13, sf::Color(240, 240, 240), 7);
        }

        if (pendingCard.active) {
            const CardDef* c = findCard(pendingCard.cardId);
            if (c) {
                const float t = std::min(1.0f, pendingCard.elapsed / pendingCard.duration);
                const float x = pendingCard.fromRect.left + (pendingCard.target.x - pendingCard.fromRect.left) * t;
                const float y = pendingCard.fromRect.top + (pendingCard.target.y - pendingCard.fromRect.top) * t;
                const float scale = 1.0f - 0.25f * t;

                sf::RectangleShape card(sf::Vector2f(pendingCard.fromRect.width * scale, pendingCard.fromRect.height * scale));
                card.setPosition(x, y);
                card.setFillColor(cardColorWithUpgrade(c->type, pendingCard.cardId));
                card.setOutlineThickness(2.f);
                card.setOutlineColor(sf::Color::Black);
                window.draw(card);

                drawText(c->name, x + 6.f, y + 8.f, 16, cardTitleColorWithUpgrade(pendingCard.cardId));
                drawText("费 " + std::to_string(c->cost), x + 6.f, y + 30.f, 14, cardMetaColorWithUpgrade(pendingCard.cardId));
            }
        }

        if (turnBannerTimer > 0.0f) {
            const float t = std::min(1.0f, turnBannerTimer / 1.1f);
            const sf::Uint8 alpha = static_cast<sf::Uint8>(60.f + t * 140.f);
            sf::RectangleShape banner(sf::Vector2f(206.f, 42.f));
            banner.setPosition(470.f, 146.f);
            banner.setFillColor(sf::Color(55, 118, 70, alpha));
            banner.setOutlineThickness(2.f);
            banner.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));
            window.draw(banner);
            drawText("你的回合", 528.f, 154.f, 22, sf::Color(255, 255, 255, alpha));
        }

        sf::RectangleShape logPanel(sf::Vector2f(260.f, 272.f));
        logPanel.setPosition(20.f, 164.f);
        logPanel.setFillColor(sf::Color(20, 28, 36, 214));
        logPanel.setOutlineThickness(2.f);
        logPanel.setOutlineColor(sf::Color(222, 208, 164, 120));
        window.draw(logPanel);

        const float logLeft = 34.f;
        float logY = 176.f;
        drawText("战斗日志", logLeft, logY, 20, sf::Color(232, 226, 196));
        logY += 27.f;
        const float logBottom = 424.f;
        const float logLineHeight = 13.f * 1.25f;
        for (const auto& line : logs) {
            const int lines = wrappedLineCount(line, 16);
            const float nextLogY = logY + static_cast<float>(lines) * logLineHeight + 3.f;
            if (nextLogY > logBottom) {
                break;
            }
            drawWrappedText(line, logLeft, logY, 13, sf::Color(232, 232, 230), 16);
            logY = nextLogY;
        }

        for (const auto& item : floatingTexts) {
            const float t = std::min(1.0f, item.remaining / item.duration);
            sf::Color c = item.color;
            c.a = static_cast<sf::Uint8>(70.f + t * 185.f);
            drawSfText(item.text, item.position.x, item.position.y, 20, c);
        }

        drawText(std::string("T：动画") + (enableCombatAnimations ? "开" : "关"), 180.f, 680.f, 18, sf::Color(180, 180, 180));
        drawText("R：重新开始本局", 20.f, 680.f, 18, sf::Color(180, 180, 180));

        if (showOwnedDeckOverlay) {
            renderOwnedDeckOverlay();
            return;
        }

        if (showRelicOverlay) {
            renderRelicOverlay();
            return;
        }

        if (activePileOverlay != PileOverlayType::None) {
            renderPileOverlay();
        }
    }

    void renderReward() {
        drawText("选择 1 张奖励卡牌", 450.f, 80.f, 40, sf::Color(255, 255, 210));
        drawText("第 " + std::to_string(currentFloor + 1) + " 层战斗已完成", 500.f, 130.f, 24, sf::Color(220, 220, 220));

        auto rects = rewardRects();
        for (size_t i = 0; i < rewardChoices.size() && i < 3; ++i) {
            const CardDef* c = findCard(rewardChoices[i]);
            if (!c) {
                continue;
            }

            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top);
            card.setFillColor(cardColorWithUpgrade(c->type, rewardChoices[i]));
            card.setOutlineThickness(3.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c->name, rects[i].left + 10.f, rects[i].top + 10.f, 26, cardTitleColorWithUpgrade(rewardChoices[i]));
            drawText("费用：" + std::to_string(c->cost), rects[i].left + 10.f, rects[i].top + 50.f, 18, cardMetaColorWithUpgrade(rewardChoices[i]));
            drawTextureFit(*cardIcon(c->type), sf::FloatRect(rects[i].left + 166.f, rects[i].top + 10.f, 42.f, 42.f));
            drawWrappedText(c->desc, rects[i].left + 10.f, rects[i].top + 82.f, 16, sf::Color(240, 240, 240), 11);
        }

        drawButton(rects[3], "跳过", sf::Color(80, 80, 80, 230));
        drawButton(rects[4], "查看牌组", sf::Color(56, 94, 126, 230));
        drawText("点击卡牌选择或跳过", 500.f, 620.f, 24, sf::Color(230, 230, 230));

        if (activePileOverlay != PileOverlayType::None) {
            renderPileOverlay();
        }
    }

    void renderCampfire() {
        drawText("营火", 590.f, 100.f, 62, sf::Color(255, 226, 160));
        drawText("你可以在这里休整或锻造卡牌", 420.f, 190.f, 28, sf::Color(230, 230, 230));
        drawButton(campRestRect, "休息（回复30生命）", sf::Color(76, 126, 88, 235));
        drawButton(campForgeRect, "锻造（升级1张卡）", sf::Color(126, 96, 66, 235));
        if (campfireDidAction) {
            drawText("本层营火已操作，点击按钮将无效", 410.f, 450.f, 24, sf::Color(255, 210, 170));
        }
    }

    void renderShop() {
        drawText("商店", 590.f, 70.f, 58, sf::Color(255, 232, 176));
        drawText("金币：" + std::to_string(gold), 80.f, 120.f, 30, sf::Color(245, 232, 180));

        auto rects = shopCardRects();
        for (size_t i = 0; i < rects.size() && i < shopChoices.size(); ++i) {
            sf::RectangleShape box(sf::Vector2f(rects[i].width, rects[i].height));
            box.setPosition(rects[i].left, rects[i].top);
            box.setFillColor(sf::Color(36, 46, 66, 220));
            box.setOutlineThickness(2.f);
            box.setOutlineColor(sf::Color(0, 0, 0, 180));
            window.draw(box);

            if (shopChoices[i] <= 0) {
                drawText("已售出", rects[i].left + 58.f, rects[i].top + 58.f, 24, sf::Color(170, 170, 170));
                continue;
            }

            const CardDef* c = findCard(shopChoices[i]);
            if (!c) {
                continue;
            }

            drawText(c->name, rects[i].left + 10.f, rects[i].top + 6.f, 24, cardTitleColorWithUpgrade(shopChoices[i]));
            drawText("费 " + std::to_string(c->cost), rects[i].left + 10.f, rects[i].top + 38.f, 18, cardMetaColorWithUpgrade(shopChoices[i]));
            const int price = cardPrice(c->id);
            const sf::Color priceColor = (gold >= price) ? sf::Color(255, 220, 150) : sf::Color(255, 140, 140);
            drawText("价格 " + std::to_string(price), rects[i].left + 10.f, rects[i].top + 66.f, 18, priceColor);
            drawWrappedText(c->desc, rects[i].left + 10.f, rects[i].top + 94.f, 14, sf::Color(220, 220, 220), 11);
        }

        drawButton(shopLeaveRect, "离开商店（下一层）", sf::Color(72, 98, 126, 240));
        drawButton(shopPotionRect, "购买药水（" + std::to_string(potionCapacityPrice) + "金币）", sf::Color(92, 76, 136, 240));
        drawButton(shopRemoveRect, "移除1张卡（" + std::to_string(deckRemovePrice) + "金币）", sf::Color(138, 82, 72, 240));
        drawText("红色价格代表金币不足", 940.f, 380.f, 20, sf::Color(255, 160, 160));
    }

    void renderEvent() {
        if (currentEventId == 1) {
            drawText("事件：生锈刀刃", 450.f, 80.f, 54, sf::Color(250, 210, 170));
            drawWrappedText("你在前方看见了一座荒废神殿。祭坛上有一把生锈匕首，似乎在渴望鲜血。", 170.f, 180.f, 28, sf::Color(235, 235, 235), 28);
            drawButton(eventLeftRect, "失去16生命，获得 邪恶之刃", sf::Color(130, 72, 72, 240));
            drawButton(eventRightRect, "无视匕首，拿走180金币", sf::Color(92, 118, 72, 240));
            drawText("选项影响：左-16生命，右+180金币", 410.f, 650.f, 22, sf::Color(230, 220, 190));
            return;
        }

        drawText("事件：救与不救", 470.f, 80.f, 54, sf::Color(250, 210, 170));
        drawWrappedText("一位看上去需要帮助的老人挡在路中央。你该如何选择？", 220.f, 190.f, 30, sf::Color(235, 235, 235), 24);
        drawButton(eventLeftRect, "施以援手，获得一件遗物", sf::Color(130, 100, 72, 240));
        drawButton(eventRightRect, "漠不关心，移除2张卡", sf::Color(92, 118, 72, 240));
        drawText("选项影响：左+遗物，右移除2张卡", 408.f, 650.f, 22, sf::Color(230, 220, 190));
    }

    void renderDeckEdit() {
        drawText("卡组操作", 540.f, 40.f, 52, sf::Color(246, 232, 188));
        if (deckEditMode == DeckEditMode::UpgradeOne) {
            drawText("选择1张卡进行升级", 490.f, 98.f, 28, sf::Color(230, 230, 230));
        } else if (deckEditMode == DeckEditMode::RemoveTwo) {
            drawText("选择2张卡移除，剩余：" + std::to_string(deckEditRemaining), 430.f, 98.f, 28, sf::Color(230, 230, 230));
        } else {
            drawText("选择1张卡移除", 520.f, 98.f, 28, sf::Color(230, 230, 230));
        }

        if (masterDeck.empty()) {
            drawText("当前牌组为空", 560.f, 130.f, 24, sf::Color(255, 180, 180));
        }

        auto rects = deckEditRects();
        for (size_t i = 0; i < rects.size(); ++i) {
            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top);
            const CardDef* c = findCard(masterDeck[i]);
            card.setFillColor(c ? cardColorWithUpgrade(c->type, masterDeck[i]) : sf::Color(90, 90, 90));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(sf::Color(0, 0, 0, 190));
            window.draw(card);

            if (!c) {
                continue;
            }
            drawText(c->name, rects[i].left + 6.f, rects[i].top + 6.f, 18, cardTitleColorWithUpgrade(masterDeck[i]));
            drawText("费 " + std::to_string(c->cost), rects[i].left + 6.f, rects[i].top + 32.f, 14, cardMetaColorWithUpgrade(masterDeck[i]));
            drawWrappedText(c->desc, rects[i].left + 6.f, rects[i].top + 50.f, 12, sf::Color(230, 230, 230), 16);
        }

        drawButton(deckDoneRect, "完成", sf::Color(76, 110, 140, 240));
    }

    /**
     * @brief 渲染主菜单
     * 
     * 显示内容：
     * - 游戏Logo和标题
     * - 故事背景介绍文本
     * - 开始游戏/玩法介绍/退出游戏按钮
     * - 累计通关次数
     */
    void renderMainMenu() {
        drawTextureFit(logoTex, sf::FloatRect(430.f, 54.f, 96.f, 96.f));
        drawText("尖塔战士", 534.f, 58.f, 72, sf::Color(35, 28, 18, 190));
        drawText("尖塔战士", 528.f, 52.f, 72, sf::Color(255, 246, 214));
        drawCenteredWrappedText(storylineIntro(), 236.f, 20, sf::Color(232, 224, 204), 60);
        drawCenteredWrappedText(storylineGoal(), 272.f, 19, sf::Color(220, 220, 198), 60);

        drawButton(menuStartRect, "开始游戏", sf::Color(58, 135, 86, 235));
        drawButton(menuRulesRect, "玩法介绍", sf::Color(56, 94, 126, 235));
        drawButton(menuExitRect, "退出游戏", sf::Color(146, 72, 72, 235));
        drawTextureFit(startTex, sf::FloatRect(menuStartRect.left + 30.f, menuStartRect.top + 22.f, 38.f, 38.f));
        drawTextureFit(exitTex, sf::FloatRect(menuExitRect.left + 30.f, menuExitRect.top + 22.f, 38.f, 38.f));

        drawText("累计通关：" + std::to_string(totalWins), 20.f, 680.f, 20, sf::Color(220, 220, 180));
    }

    /**
     * @brief 渲染玩法介绍页面
     * 
     * 显示内容：
     * - 游戏目标
     * - 卡牌类型说明
     * - 战斗流程
     * - 快捷键说明
     * - 返回按钮
     */
    void renderRules() {
        drawText("玩法介绍", 520.f, 40.f, 56, sf::Color(255, 246, 214));
        
        float y = 120.f;
        const float lineHeight = 28.f;
        const float sectionGap = 16.f;
        
        drawText("【游戏目标】", 100.f, y, 26, sf::Color(255, 220, 150));
        y += lineHeight + 4.f;
        drawText("击败15层的Boss，通关游戏。", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight + sectionGap;
        
        drawText("【卡牌类型】", 100.f, y, 26, sf::Color(255, 220, 150));
        y += lineHeight + 4.f;
        drawText("攻击牌（红）：造成伤害", 120.f, y, 22, sf::Color(255, 150, 150));
        y += lineHeight;
        drawText("技能牌（蓝）：获得格挡或效果", 120.f, y, 22, sf::Color(150, 180, 255));
        y += lineHeight;
        drawText("能力牌（黄）：持续生效的增益", 120.f, y, 22, sf::Color(255, 255, 150));
        y += lineHeight + sectionGap;
        
        drawText("【战斗流程】", 100.f, y, 26, sf::Color(255, 220, 150));
        y += lineHeight + 4.f;
        drawText("1. 抽牌阶段：每回合抽5张牌", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("2. 出牌阶段：消耗能量打出卡牌", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("3. 结束回合：弃掉所有手牌，敌人行动", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight + sectionGap;
        
        drawText("【状态效果】", 100.f, y, 26, sf::Color(255, 220, 150));
        y += lineHeight + 4.f;
        drawText("力量：增加攻击伤害", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("格挡：减少受到的伤害", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("虚弱：造成的伤害减少25%", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("易伤：受到的伤害增加50%", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight + sectionGap;
        
        drawText("【快捷键】", 100.f, y, 26, sf::Color(255, 220, 150));
        y += lineHeight + 4.f;
        drawText("R - 重置当前运行", 120.f, y, 22, sf::Color(220, 220, 220));
        y += lineHeight;
        drawText("T - 切换动画效果", 120.f, y, 22, sf::Color(220, 220, 220));
        
        drawButton(backMenuRect, "返回主菜单", sf::Color(86, 102, 145, 240));
    }

    void renderBgmToggle() {
        sf::RectangleShape btn(sf::Vector2f(bgmToggleRect.width, bgmToggleRect.height));
        btn.setPosition(bgmToggleRect.left, bgmToggleRect.top);
        btn.setFillColor(bgmEnabled ? sf::Color(56, 94, 126, 220) : sf::Color(80, 60, 60, 220));
        btn.setOutlineThickness(1.f);
        btn.setOutlineColor(sf::Color(0, 0, 0, 180));
        window.draw(btn);

        float speakerX = bgmToggleRect.left - 18.f;
        float speakerY = bgmToggleRect.top + 3.f;
        sf::Color speakerColor = bgmEnabled ? sf::Color(200, 220, 255) : sf::Color(150, 130, 130);

        sf::ConvexShape speaker;
        speaker.setPointCount(4);
        speaker.setPoint(0, sf::Vector2f(speakerX, speakerY + 4.f));
        speaker.setPoint(1, sf::Vector2f(speakerX + 6.f, speakerY + 4.f));
        speaker.setPoint(2, sf::Vector2f(speakerX + 6.f, speakerY + 12.f));
        speaker.setPoint(3, sf::Vector2f(speakerX, speakerY + 12.f));
        speaker.setFillColor(speakerColor);
        window.draw(speaker);

        sf::ConvexShape horn;
        horn.setPointCount(4);
        horn.setPoint(0, sf::Vector2f(speakerX + 6.f, speakerY + 2.f));
        horn.setPoint(1, sf::Vector2f(speakerX + 14.f, speakerY - 2.f));
        horn.setPoint(2, sf::Vector2f(speakerX + 14.f, speakerY + 18.f));
        horn.setPoint(3, sf::Vector2f(speakerX + 6.f, speakerY + 14.f));
        horn.setFillColor(speakerColor);
        window.draw(horn);

        drawText(bgmEnabled ? "开" : "关", bgmToggleRect.left + 12.f, bgmToggleRect.top + 2.f, 12, sf::Color::White);
    }

    /**
     * @brief 渲染开发者测试按钮
     * 
     * 显示内容：
     * - 小型按钮，显示"测试"
     * - 点击后对敌人造成999伤害
     */
    void renderDevTest() {
        sf::RectangleShape btn(sf::Vector2f(devTestRect.width, devTestRect.height));
        btn.setPosition(devTestRect.left, devTestRect.top);
        btn.setFillColor(sf::Color(120, 60, 60, 220));
        btn.setOutlineThickness(1.f);
        btn.setOutlineColor(sf::Color(0, 0, 0, 180));
        window.draw(btn);
        drawText("测试", devTestRect.left + 6.f, devTestRect.top + 2.f, 12, sf::Color::White);
    }

    void renderDefeat() {
        drawText("战败", 560.f, 200.f, 84, sf::Color(240, 120, 120));
        drawText("本次挑战未能击败 BOSS", 430.f, 320.f, 34, sf::Color(230, 230, 230));
        drawButton(backMenuRect, "返回主菜单", sf::Color(86, 102, 145, 240));
    }

    void renderVictoryThanks() {
        drawTextureFit(thanksTex, sf::FloatRect(545.f, 170.f, 190.f, 190.f));
        drawTextCentered("感谢游玩", 110.f, 64, sf::Color(125, 240, 170));
        drawTextCentered("尖塔的恶魔被你击败！", 410.f, 30, sf::Color(236, 236, 236));
        drawTextCentered("按任意键进入制作人名单", 470.f, 24, sf::Color(210, 210, 220));
    }

    void renderCredits() {
        drawTextCentered("制作人名单", 90.f, 62, sf::Color(250, 230, 170));
        drawTextCentered("策划：杨立鑫", 200.f, 34, sf::Color(235, 235, 235));
        drawTextCentered("程序：杨立鑫", 250.f, 34, sf::Color(235, 235, 235));
        drawTextCentered("美术：杨立鑫", 300.f, 34, sf::Color(235, 235, 235));
        drawTextCentered("音频：杨立鑫", 350.f, 34, sf::Color(235, 235, 235));
        drawTextCentered("测试：宋金林", 400.f, 34, sf::Color(235, 235, 235));
        drawButton(backMenuRect, "返回主菜单", sf::Color(86, 102, 145, 240));
    }

    /**
     * @brief 主渲染函数
     * 
     * 渲染流程：
     * 1. 清空屏幕（深色背景）
     * 2. 绘制场景背景
     * 3. 根据当前阶段调用对应渲染函数
     * 4. 绘制BGM开关按钮
     * 5. 绘制浮动提示文字
     * 6. 显示窗口内容
     * 
     * BGM切换逻辑：
     * - 不同阶段播放不同的背景音乐
     * - Boss胜利使用特殊BGM
     */
    void render() {
        window.clear(sf::Color(14, 24, 30));

        // Draw scene-specific background
        if (phase == Phase::MainMenu) {
            drawBackground(bgMainMenu);
        } else if (phase == Phase::Battle || phase == Phase::Reward) {
            drawBackground(bgBattle);
        } else if (phase == Phase::Campfire) {
            drawBackground(bgCampfire);
        } else if (phase == Phase::Shop) {
            drawBackground(bgShop);
        } else if (phase == Phase::Event) {
            if (currentEventId == 1) {
                drawBackground(bgEvent1);
            } else {
                drawBackground(bgEvent2);
            }
        } else if (phase == Phase::Defeat) {
            drawBackground(bgDefeat);
        } else if (phase == Phase::VictoryThanks || phase == Phase::Credits) {
            drawBackground(bgVictory);
        } else {
            drawBackground(bgBattle);
        }

        // Subtle overlay for text readability
        sf::VertexArray bg(sf::Quads, 4);
        bg[0].position = sf::Vector2f(0.f, 0.f);
        bg[1].position = sf::Vector2f(1280.f, 0.f);
        bg[2].position = sf::Vector2f(1280.f, 720.f);
        bg[3].position = sf::Vector2f(0.f, 720.f);
        bg[0].color = sf::Color(14, 24, 30, 120);
        bg[1].color = sf::Color(14, 24, 30, 120);
        bg[2].color = sf::Color(14, 24, 30, 120);
        bg[3].color = sf::Color(14, 24, 30, 120);
        window.draw(bg);

        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(std::clock()) * 0.0018f);
        sf::CircleShape glowA(220.f);
        glowA.setPosition(920.f, -90.f);
        glowA.setFillColor(sf::Color(255, 198, 104, static_cast<sf::Uint8>(22 + pulse * 26.f)));
        window.draw(glowA);

        sf::CircleShape glowB(280.f);
        glowB.setPosition(-120.f, 420.f);
        glowB.setFillColor(sf::Color(84, 188, 182, static_cast<sf::Uint8>(16 + pulse * 20.f)));
        window.draw(glowB);

        // Play appropriate BGM
        if (phase == Phase::MainMenu) {
            if (currentBgmPath != "assets/audio/bgm/main_menu.mp3") playBgm("assets/audio/bgm/main_menu.mp3");
        } else if (phase == Phase::Battle) {
            if (currentBgmPath != "assets/audio/bgm/battle.mp3") playBgm("assets/audio/bgm/battle.mp3");
        } else if (phase == Phase::Campfire) {
            if (currentBgmPath != "assets/audio/bgm/campfire.mp3") playBgm("assets/audio/bgm/campfire.mp3");
        } else if (phase == Phase::Shop) {
            if (currentBgmPath != "assets/audio/bgm/shop.mp3") playBgm("assets/audio/bgm/shop.mp3");
        } else if (phase == Phase::Event) {
            if (currentBgmPath != "assets/audio/bgm/event.mp3") playBgm("assets/audio/bgm/event.mp3");
        } else if (phase == Phase::Defeat) {
            if (currentBgmPath != "assets/audio/bgm/defeat.mp3") playBgm("assets/audio/bgm/defeat.mp3");
        } else if (phase == Phase::VictoryThanks) {
            if (currentBgmPath.find("victory_boss") == std::string::npos) playBgm("assets/audio/bgm/victory_boss");
        } else if (phase == Phase::Credits) {
            if (currentBgmPath != "assets/audio/bgm/victory.mp3") playBgm("assets/audio/bgm/victory.mp3");
        }

        if (phase == Phase::MainMenu) {
            renderMainMenu();
        } else if (phase == Phase::Rules) {
            renderRules();
        } else if (phase == Phase::Battle) {
            renderBattle();
        } else if (phase == Phase::Reward) {
            renderReward();
        } else if (phase == Phase::Campfire) {
            renderCampfire();
        } else if (phase == Phase::Shop) {
            renderShop();
        } else if (phase == Phase::Event) {
            renderEvent();
        } else if (phase == Phase::DeckEdit) {
            renderDeckEdit();
        } else if (phase == Phase::VictoryThanks) {
            renderVictoryThanks();
        } else if (phase == Phase::Credits) {
            renderCredits();
        } else if (phase == Phase::Defeat) {
            renderDefeat();
        }

        if (phase != Phase::MainMenu) {
            renderBgmToggle();
        }
        if (phase == Phase::Battle) {
            renderDevTest();
        }
        renderActionHint();

        window.display();
    }
};

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--self-test") {
        return Game::runSelfTest(std::cout) ? 0 : 2;
    }

    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
