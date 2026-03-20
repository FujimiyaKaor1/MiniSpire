#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

enum class CardType { Attack, Skill, Power };
enum class Phase { MainMenu, Battle, Reward, Campfire, Shop, Event, DeckEdit, VictoryThanks, Credits, Defeat };
enum class RoomType { Battle, Campfire, Shop, Event };
enum class DeckEditMode { None, UpgradeOne, RemoveOne, RemoveTwo };

struct CardDef {
    int id;
    std::string name;
    CardType type;
    int cost;
    int damage;
    int hits;
    int block;
    int gainStrength;
    int applyVulnerable;
    int applyWeak;
    int draw;
    int gainEnergy;
    int loseHp;
    bool exhaust;
    bool grantsStrengthPerTurn;
    std::string desc;
    bool healFromDamage = false;
};

struct FloorNode {
    RoomType type = RoomType::Battle;
    int enemyIndex = -1;
    int eventId = 0;
};

struct Intent {
    std::string label;
    int damage;
    int hits;
    int block;
    int gainStrength;
    int applyWeak;
    int applyVulnerable;
};

struct EnemyDef {
    std::string name;
    int maxHp;
    bool elite;
    bool boss;
    std::vector<Intent> pattern;
};

struct Combatant {
    int hp = 0;
    int maxHp = 0;
    int block = 0;
    int strength = 0;
    int weak = 0;
    int vulnerable = 0;
};

struct FloatingText {
    sf::String text;
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float remaining = 0.0f;
    float duration = 0.0f;
};

struct PendingCardPlay {
    bool active = false;
    int handIndex = -1;
    int cardId = -1;
    float elapsed = 0.0f;
    float duration = 0.16f;
    sf::FloatRect fromRect;
    sf::Vector2f target;
};

struct PendingEnemyTurn {
    bool active = false;
    bool resolved = false;
    float elapsed = 0.0f;
    float windup = 0.22f;
    float recover = 0.12f;
};

struct AppConfig {
    unsigned int maxFps = 60;
    bool enableAnimations = true;
    bool enableFloatingText = true;
    float textScale = 1.0f;
};

class Game {
public:
    static std::vector<FloorNode> defaultFloorPlan() {
        return {
            {RoomType::Battle, 0, 0},
            {RoomType::Battle, 1, 0},
            {RoomType::Campfire, -1, 0},
            {RoomType::Battle, 2, 0},
            {RoomType::Battle, 4, 0},
            {RoomType::Shop, -1, 0},
            {RoomType::Battle, 3, 0},
            {RoomType::Event, -1, 1},
            {RoomType::Campfire, -1, 0},
            {RoomType::Battle, 5, 0},
            {RoomType::Shop, -1, 0},
            {RoomType::Battle, 6, 0},
            {RoomType::Event, -1, 2},
            {RoomType::Campfire, -1, 0},
            {RoomType::Battle, 7, 0}
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
        expensive.grantsStrengthPerTurn = true;
        expensive.exhaust = false;
        expect(calculateCardPrice(expensive) == 124, "shop price high value card");

        CardDef bounded{};
        bounded.cost = 10;
        bounded.damage = 40;
        bounded.block = 0;
        bounded.draw = 0;
        bounded.grantsStrengthPerTurn = true;
        bounded.exhaust = false;
        expect(calculateCardPrice(bounded) == 180, "shop price upper clamp");

        expect(battleGoldForFloor(0) == 35, "battle gold floor 1");
        expect(battleGoldForFloor(5) == 50, "battle gold floor 6");
        expect(battleGoldForFloor(14) == 75, "battle gold floor 15 cap");

        expect(isValidEventChoice(1, 0), "event 1 option 0 valid");
        expect(isValidEventChoice(1, 1), "event 1 option 1 valid");
        expect(isValidEventChoice(2, 0), "event 2 option 0 valid");
        expect(isValidEventChoice(2, 1), "event 2 option 1 valid");
        expect(!isValidEventChoice(1, 2), "event invalid option rejected");
        expect(!isValidEventChoice(99, 0), "unknown event rejected");

        out << (success ? "SELF_TEST_PASS" : "SELF_TEST_FAIL") << '\n';
        return success;
    }

    Game()
        : window(sf::VideoMode(1280, 720), "迷你尖塔 Demo"),
          rng(static_cast<std::mt19937::result_type>(std::random_device{}())) {
                loadConfig();
                enableCombatAnimations = config.enableAnimations;
                window.setFramerateLimit(config.maxFps);
        loadFont();
        loadTextures();
        initCards();
        initEnemies();
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
    sf::RenderWindow window;
    sf::Font font;
    bool hasFont = false;

    sf::Texture cardAttackTex;
    sf::Texture cardSkillTex;
    sf::Texture cardPowerTex;
    sf::Texture enemy1Tex;
    sf::Texture enemy2Tex;
    sf::Texture enemy3Tex;
    sf::Texture eliteTex;
    sf::Texture bossTex;
    sf::Texture logoTex;
    sf::Texture intentTex;
    sf::Texture startTex;
    sf::Texture exitTex;
    sf::Texture thanksTex;

    std::mt19937 rng;

    std::vector<CardDef> cardPool;
    std::unordered_map<int, CardDef> cardsById;
    std::vector<int> baseCardIds;
    std::vector<EnemyDef> enemySequence;
    std::vector<FloorNode> floorPlan;
    std::vector<int> shopChoices;

    Phase phase = Phase::MainMenu;
    int currentFloor = 0;
    int currentEnemyIndex = 0;
    int currentEventId = 0;
    float phaseTimer = 0.0f;

    Combatant player;
    Combatant enemy;
    int enemyIntentIndex = 0;

    int playerEnergy = 3;
    int playerStrengthPerTurn = 0;
    int totalWins = 0;
    int gold = 99;
    int potionCount = 1;
    int potionMax = 4;
    int potionCapacityPrice = 85;
    int deckRemovePrice = 70;
    bool campfireDidAction = false;
    DeckEditMode deckEditMode = DeckEditMode::None;
    int deckEditRemaining = 0;

    std::vector<int> masterDeck;
    std::vector<int> drawPile;
    std::vector<int> discardPile;
    std::vector<int> hand;
    std::vector<int> exhaustPile;

    std::vector<int> rewardChoices;
    std::deque<std::string> logs;
    std::vector<FloatingText> floatingTexts;
    PendingCardPlay pendingCard;
    PendingEnemyTurn pendingEnemy;
    AppConfig config;

    sf::FloatRect endTurnRect{1070.f, 620.f, 170.f, 70.f};
    sf::FloatRect potionRect{1040.f, 530.f, 200.f, 72.f};
    sf::FloatRect menuStartRect{470.f, 320.f, 340.f, 82.f};
    sf::FloatRect menuExitRect{470.f, 430.f, 340.f, 82.f};
    sf::FloatRect backMenuRect{470.f, 560.f, 340.f, 72.f};
    sf::FloatRect campRestRect{270.f, 320.f, 320.f, 90.f};
    sf::FloatRect campForgeRect{700.f, 320.f, 320.f, 90.f};
    sf::FloatRect shopPotionRect{940.f, 500.f, 280.f, 70.f};
    sf::FloatRect shopRemoveRect{940.f, 590.f, 280.f, 70.f};
    sf::FloatRect shopLeaveRect{940.f, 410.f, 280.f, 70.f};
    sf::FloatRect eventLeftRect{180.f, 500.f, 430.f, 120.f};
    sf::FloatRect eventRightRect{670.f, 500.f, 430.f, 120.f};
    sf::FloatRect deckDoneRect{1000.f, 620.f, 230.f, 70.f};

    float enemyHitFlashTimer = 0.0f;
    float playerHitFlashTimer = 0.0f;
    float turnBannerTimer = 0.0f;
    int hoveredCardIndex = -1;
    bool enableCombatAnimations = true;
    Phase deckEditReturnPhase = Phase::MainMenu;
    bool deckEditAdvanceAfterDone = false;
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
    }

    void loadFont() {
        hasFont = font.loadFromFile("assets/fonts/NotoSansCJKsc-Regular.otf") ||
                  font.loadFromFile("assets/font.ttf") ||
                  font.loadFromFile("C:/Windows/Fonts/arial.ttf");
        if (!hasFont) {
            std::cerr << "[warn] 字体加载失败，文本将无法显示" << '\n';
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
        loadTexture(eliteTex, "assets/images/enemies/elite.png");
        loadTexture(bossTex, "assets/images/enemies/boss.png");

        loadTexture(logoTex, "assets/images/ui/logo.png");
        loadTexture(intentTex, "assets/images/ui/intent.png");
        loadTexture(startTex, "assets/images/ui/start.png");
        loadTexture(exitTex, "assets/images/ui/exit.png");
        loadTexture(thanksTex, "assets/images/ui/thanks.png");
    }

    void pushLog(const std::string& line) {
        logs.push_back(line);
        while (logs.size() > 12) {
            logs.pop_front();
        }
    }

    void initCards() {
        cardsById.clear();
        cardPool.clear();
        baseCardIds.clear();

        cardPool = {
            {1, "打击", CardType::Attack, 1, 6, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 6 点伤害"},
            {2, "防御", CardType::Skill, 1, 0, 1, 5, 0, 0, 0, 0, 0, 0, false, false, "获得 5 点格挡"},
            {3, "重击", CardType::Attack, 2, 8, 1, 0, 0, 2, 0, 0, 0, 0, false, false, "造成 8 点伤害，施加 2 层易伤"},
            {4, "重斩", CardType::Attack, 2, 14, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 14 点伤害"},
            {5, "立盾", CardType::Skill, 1, 0, 1, 8, 0, 0, 0, 0, 0, 0, false, false, "获得 8 点格挡"},
            {6, "怒火", CardType::Skill, 1, 0, 1, 0, 2, 0, 0, 0, 0, 0, false, false, "获得 2 点力量"},
            {7, "震击", CardType::Attack, 1, 9, 1, 0, 0, 0, 0, 1, 0, 0, false, false, "造成 9 点伤害，抽 1 张牌"},
            {8, "坚毅", CardType::Skill, 1, 0, 1, 9, 0, 0, 0, 0, 0, 0, true, false, "获得 9 点格挡，消耗"},
            {9, "横扫", CardType::Attack, 1, 7, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 7 点伤害"},
            {10, "战斗恍惚", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, true, false, "抽 2 张牌，消耗"},
            {11, "铁波", CardType::Attack, 1, 5, 1, 5, 0, 0, 0, 0, 0, 0, false, false, "造成 5 点伤害并获得 5 点格挡"},
            {12, "强行防御", CardType::Skill, 1, 0, 1, 14, 0, 0, 0, 0, 0, 0, false, false, "获得 14 点格挡"},
            {13, "耸肩无视", CardType::Skill, 1, 0, 1, 8, 0, 0, 0, 1, 0, 0, false, false, "获得 8 点格挡，抽 1 张牌"},
            {14, "上勾拳", CardType::Attack, 2, 11, 1, 0, 0, 1, 1, 0, 0, 0, false, false, "造成 11 点伤害，施加 1 层虚弱和 1 层易伤"},
            {15, "乱剑雨", CardType::Attack, 1, 4, 3, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 4 点伤害，共 3 次"},
            {16, "极限突破", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "将你的力量翻倍，消耗"},
            {17, "暴走", CardType::Attack, 1, 10, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 10 点伤害"},
            {18, "献祭", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 3, 1, 6, true, false, "失去 6 点生命，获得 1 点能量并抽 3 张牌，消耗"},
            {19, "壁垒", CardType::Skill, 2, 0, 1, 30, 0, 0, 0, 0, 0, 0, true, false, "获得 30 点格挡，消耗"},
            {20, "恶魔形态", CardType::Power, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, true, "每回合开始获得 1 点力量，消耗"},

            {21, "快速检索", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, true, false, "抽 1 张牌，消耗"},
            {22, "战术研读", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 2, 0, 0, false, false, "抽 2 张牌"},
            {23, "沉着", CardType::Skill, 1, 0, 1, 6, 0, 0, 0, 1, 0, 0, false, false, "获得 6 点格挡并抽 1 张牌"},
            {24, "冲刺", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 3, 0, 0, true, false, "抽 3 张牌，消耗"},
            {25, "窥探弱点", CardType::Skill, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, false, false, "施加 1 层易伤并抽 1 张牌"},
            {26, "武器整备", CardType::Skill, 1, 0, 1, 5, 1, 0, 0, 0, 0, 0, false, false, "获得 5 点格挡并获得 1 点力量"},
            {27, "疾风连抽", CardType::Skill, 2, 0, 1, 0, 0, 0, 0, 4, 0, 0, true, false, "抽 4 张牌，消耗"},
            {28, "应急补给", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 2, 1, 0, true, false, "抽 2 张牌并获得 1 点能量，消耗"},

            {29, "战斗专注", CardType::Power, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, true, true, "每回合开始获得 1 点力量，消耗"},
            {30, "护体术", CardType::Skill, 1, 0, 1, 12, 0, 0, 0, 0, 0, 0, true, false, "获得 12 点格挡，消耗"},
            {31, "穿刺", CardType::Attack, 1, 12, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "造成 12 点伤害，消耗"},
            {32, "双重格挡", CardType::Skill, 2, 0, 1, 18, 0, 0, 0, 0, 0, 0, true, false, "获得 18 点格挡，消耗"},
            {33, "痛击", CardType::Attack, 2, 16, 1, 0, 0, 1, 0, 0, 0, 0, true, false, "造成 16 点伤害并施加 1 层易伤，消耗"},
            {34, "激昂", CardType::Skill, 1, 0, 1, 0, 2, 0, 0, 0, 0, 0, true, false, "获得 2 点力量，消耗"},
            {35, "稳固阵线", CardType::Skill, 1, 0, 1, 10, 0, 0, 0, 0, 0, 0, true, false, "获得 10 点格挡，消耗"},
            {36, "邪恶之刃", CardType::Attack, 2, 10, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "造成 10 点伤害并回复等量生命"}
        };

        for (auto& c : cardPool) {
            if (c.id == 36) {
                c.healFromDamage = true;
            }
        }

        std::vector<CardDef> upgradedCards;
        upgradedCards.reserve(cardPool.size());
        for (const auto& base : cardPool) {
            baseCardIds.push_back(base.id);

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
            up.desc += "（升级）";
            upgradedCards.push_back(up);
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
            "普通敌人：黏液斗士", 35, false, false,
            {{"攻击 7", 7, 1, 0, 0, 0, 0}, {"格挡 6 + 攻击 5", 5, 1, 6, 0, 0, 0}, {"攻击 7", 7, 1, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：邪教徒", 48, false, false,
            {{"强化 +2 力量", 0, 1, 0, 2, 0, 0}, {"攻击 6 x2", 6, 2, 0, 0, 0, 0}, {"攻击 10", 10, 1, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：寄生突袭者", 50, false, false,
            {{"施加虚弱 1 + 攻击 6", 6, 1, 0, 0, 1, 0}, {"攻击 8", 8, 1, 0, 0, 0, 0}, {"格挡 8", 0, 1, 8, 0, 0, 0}}});

        enemySequence.push_back({
            "普通敌人：刀盾卫兵", 54, false, false,
            {{"攻击 8", 8, 1, 0, 0, 0, 0}, {"格挡 10", 0, 1, 10, 0, 0, 0}, {"攻击 6 x2", 6, 2, 0, 0, 0, 0}}});

        enemySequence.push_back({
            "精英敌人：装甲骑士", 74, true, false,
            {{"格挡 10 + 强化 +1 力量", 0, 1, 10, 1, 0, 0}, {"攻击 11", 11, 1, 0, 0, 0, 0}, {"攻击 7 x2", 7, 2, 0, 0, 0, 0}, {"施加易伤 1 + 攻击 8", 8, 1, 0, 0, 0, 1}}});

        enemySequence.push_back({
            "精英敌人：鲜血斗士", 88, true, false,
            {{"强化 +2 力量", 0, 1, 0, 2, 0, 0}, {"攻击 13", 13, 1, 0, 0, 0, 0}, {"攻击 8 x2", 8, 2, 0, 0, 0, 0}, {"施加虚弱 2", 0, 1, 0, 0, 2, 0}}});

        enemySequence.push_back({
            "精英敌人：诅咒祭司", 100, true, false,
            {{"施加易伤 2 + 攻击 7", 7, 1, 0, 0, 0, 2}, {"格挡 14", 0, 1, 14, 0, 0, 0}, {"攻击 12", 12, 1, 0, 0, 0, 0}, {"施加虚弱 2 + 攻击 7", 7, 1, 0, 0, 2, 0}}});

        enemySequence.push_back({
            "BOSS：尖塔守卫", 120, false, true,
            {{"强化 +2 力量", 0, 1, 0, 2, 0, 0}, {"攻击 14", 14, 1, 0, 0, 0, 0}, {"攻击 8 x2", 8, 2, 0, 0, 0, 0}, {"格挡 16 + 施加虚弱 1", 0, 1, 16, 0, 1, 0}, {"施加易伤 2 + 攻击 10", 10, 1, 0, 0, 0, 2}}});

        floorPlan = defaultFloorPlan();
    }

    void loadProgress() {
        std::ifstream in(kSavePath);
        if (!in) {
            return;
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("wins=", 0) == 0) {
                try {
                    totalWins = std::max(0, std::stoi(line.substr(5)));
                } catch (...) {
                    totalWins = 0;
                }
            }
        }
    }

    void saveProgress() const {
        std::ofstream out(kSavePath, std::ios::trunc);
        if (!out) {
            return;
        }
        out << "wins=" << totalWins << "\n";
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
        int price = 40 + c.cost * 18;
        if (c.damage >= 12 || c.block >= 12 || c.draw >= 3 || c.grantsStrengthPerTurn) {
            price += 30;
        }
        if (c.exhaust) {
            price -= 5;
        }
        return std::clamp(price, 35, 180);
    }

    static int battleGoldForFloor(int floorIndex) {
        return std::clamp(35 + floorIndex * 3, 35, 75);
    }

    static bool isValidEventChoice(int eventId, int option) {
        if (option != 0 && option != 1) {
            return false;
        }
        return eventId == 1 || eventId == 2;
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
        if (gold < potionCapacityPrice) {
            pushLog("金币不足，无法提升药剂上限");
            showActionHint("金币不足", sf::Color(255, 170, 170));
            return;
        }
        gold -= potionCapacityPrice;
        potionMax += 1;
        pushLog("药剂上限提升至 " + std::to_string(potionMax) + "（-" + std::to_string(potionCapacityPrice) + " 金币）");
        showActionHint("药剂上限提升至 " + std::to_string(potionMax), sf::Color(180, 220, 255));
        potionCapacityPrice += 45;
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
                player.hp = std::max(0, player.hp - 10);
                masterDeck.push_back(36);
                pushLog("你失去 10 点生命，并获得 邪恶之刃");
                if (player.hp <= 0) {
                    transitionTo(Phase::Defeat, "事件中生命归零");
                    return;
                }
                showActionHint("失去10生命，获得邪恶之刃", sf::Color(255, 210, 170));
                goNextFloor();
                return;
            }
            gold += 100;
            pushLog("你获得 100 金币");
            showActionHint("获得100金币", sf::Color(200, 255, 160));
            goNextFloor();
            return;
        }

        if (currentEventId == 2) {
            if (option == 0) {
                player.hp = std::max(0, player.hp - 12);
                pushLog("你损失了 12 点生命");
                if (player.hp <= 0) {
                    transitionTo(Phase::Defeat, "事件中生命归零");
                    return;
                }
                showActionHint("失去12生命", sf::Color(255, 180, 180));
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
        player.maxHp = 80;
        player.hp = 80;
        player.block = 0;
        player.strength = 0;
        player.weak = 0;
        player.vulnerable = 0;

        playerStrengthPerTurn = 0;
        gold = 99;
        potionCount = 1;
        potionMax = 4;
        potionCapacityPrice = 85;
        deckRemovePrice = 70;
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

        for (int i = 0; i < 4; ++i) {
            masterDeck.push_back(1);
        }
        for (int i = 0; i < 3; ++i) {
            masterDeck.push_back(2);
        }
        for (int i = 0; i < 2; ++i) {
            masterDeck.push_back(5);
        }
        masterDeck.push_back(3);
        masterDeck.push_back(21);
        masterDeck.push_back(22);

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

        player.block = 0;
        player.strength = 0;
        player.weak = 0;
        player.vulnerable = 0;
        playerStrengthPerTurn = 0;

        enemyIntentIndex = 0;

        const EnemyDef& def = enemySequence[currentEnemyIndex];
        enemy.maxHp = def.maxHp;
        enemy.hp = def.maxHp;
        enemy.block = 0;
        enemy.strength = 0;
        enemy.weak = 0;
        enemy.vulnerable = 0;

        pushLog("战斗开始：" + def.name);
        startPlayerTurn();
    }

    void startPlayerTurn() {
        player.block = 0;
        playerEnergy = 3;
        player.strength += playerStrengthPerTurn;
        drawCards(5);
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
        } else {
            enemyHitFlashTimer = 0.22f;
            spawnFloatingText("-" + std::to_string(damage), sf::Vector2f(620.f, 238.f), sf::Color(255, 210, 120), 0.65f);
        }
    }

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

        if (c->grantsStrengthPerTurn) {
            playerStrengthPerTurn += 1;
            pushLog("能力生效：每回合开始获得 1 点力量");
        }

        if (c->damage > 0 && enemy.hp > 0) {
            for (int i = 0; i < c->hits; ++i) {
                const int dmg = adjustedDamage(c->damage, player, enemy);
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
            }
        }

        if (c->block > 0) {
            player.block += c->block;
            pushLog(c->name + " 提供 " + std::to_string(c->block) + " 点格挡");
            spawnFloatingText("+" + std::to_string(c->block) + " 格挡", sf::Vector2f(260.f, 142.f), sf::Color(120, 210, 255), 0.8f);
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

        if (c->gainEnergy > 0) {
            playerEnergy += c->gainEnergy;
            pushLog(c->name + " 使你获得 " + std::to_string(c->gainEnergy) + " 点能量");
        }

        if (c->loseHp > 0) {
            player.hp = std::max(0, player.hp - c->loseHp);
            pushLog(c->name + " 使你失去 " + std::to_string(c->loseHp) + " 点生命");
        }

        if (c->draw > 0) {
            drawCards(c->draw);
            pushLog(c->name + " 让你抽取 " + std::to_string(c->draw) + " 张牌");
        }

        hand.erase(hand.begin() + handIndex);
        if (c->exhaust) {
            exhaustPile.push_back(cardId);
        } else {
            discardPile.push_back(cardId);
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
        const int battleGold = battleGoldForFloor(currentFloor);
        gold += battleGold;
        pushLog("获得 " + std::to_string(battleGold) + " 金币与 1 瓶药剂（当前金币 " + std::to_string(gold) + "）");
        showActionHint("战斗奖励：+" + std::to_string(battleGold) + " 金币", sf::Color(200, 255, 170));
        if (currentFloor == static_cast<int>(floorPlan.size()) - 1) {
            totalWins++;
            saveProgress();
            transitionTo(Phase::VictoryThanks, "击败最终BOSS");
            phaseTimer = 0.0f;
            return;
        }

        rewardChoices.clear();
        std::uniform_int_distribution<int> dist(0, static_cast<int>(cardPool.size()) - 1);
        while (rewardChoices.size() < 3) {
            int id = cardPool[dist(rng)].id;
            if (id == 1 || id == 2) {
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
            for (int i = 0; i < intent.hits; ++i) {
                const int dmg = adjustedDamage(intent.damage, enemy, player);
                dealDamage(player, dmg, true);
                pushLog("敌人造成 " + std::to_string(dmg) + " 点伤害");
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
        const float cardW = 110.f;
        const float cardH = 220.f;
        const float gap = 6.f;
        const float totalW = static_cast<float>(hand.size()) * cardW +
                             static_cast<float>(std::max(0, static_cast<int>(hand.size()) - 1)) * gap;
        float startX = (1280.f - totalW) * 0.5f;
        const float y = 455.f;
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

    void pickReward(int index) {
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

        goNextFloor();
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
            if (phase == Phase::Battle || phase == Phase::Reward || phase == Phase::Defeat) {
                resetRun();
            }
            return;
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

        if (phase == Phase::MainMenu) {
            if (menuStartRect.contains(mouse)) {
                resetRun();
                return;
            }
            if (menuExitRect.contains(mouse)) {
                window.close();
                return;
            }
        } else if (phase == Phase::Battle) {
            if (pendingCard.active || pendingEnemy.active) {
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
                endPlayerTurn();
                return;
            }

            auto rects = handRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    beginCardPlay(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Reward) {
            auto rects = rewardRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    pickReward(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Campfire) {
            if (campRestRect.contains(mouse)) {
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
                goNextFloor();
                return;
            }
            if (shopPotionRect.contains(mouse)) {
                tryBuyPotionCapacity();
                return;
            }
            if (shopRemoveRect.contains(mouse)) {
                tryBuyRemoveService();
                return;
            }

            auto rects = shopCardRects();
            for (size_t i = 0; i < rects.size() && i < shopChoices.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    tryBuyShopCard(static_cast<int>(i));
                    return;
                }
            }
        } else if (phase == Phase::Event) {
            if (eventLeftRect.contains(mouse)) {
                resolveEventChoice(0);
                return;
            }
            if (eventRightRect.contains(mouse)) {
                resolveEventChoice(1);
                return;
            }
        } else if (phase == Phase::DeckEdit) {
            if (deckDoneRect.contains(mouse)) {
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
            phaseTimer += dt;
            if (phaseTimer >= 3.0f) {
                transitionTo(Phase::Credits, "感谢页自动过渡");
                phaseTimer = 0.0f;
            }
        } else if (phase == Phase::Credits) {
            phaseTimer += dt;
            if (phaseTimer >= 8.0f) {
                transitionTo(Phase::MainMenu, "制作人名单自动返回主菜单");
                phaseTimer = 0.0f;
            }
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

    void drawTextureFit(const sf::Texture& tex, const sf::FloatRect& rect, sf::Color tint = sf::Color::White) {
        if (tex.getSize().x == 0 || tex.getSize().y == 0) {
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
        if (currentEnemyIndex <= 0) return &enemy1Tex;
        if (currentEnemyIndex <= 1) return &enemy2Tex;
        if (currentEnemyIndex <= 3) return &enemy3Tex;
        if (currentEnemyIndex <= 6) return &eliteTex;
        return &bossTex;
    }

    sf::Color cardColor(CardType type) const {
        switch (type) {
            case CardType::Attack: return sf::Color(190, 80, 80);
            case CardType::Skill: return sf::Color(80, 130, 190);
            case CardType::Power: return sf::Color(190, 160, 70);
            default: return sf::Color(140, 140, 140);
        }
    }

    void drawButton(const sf::FloatRect& rect, const std::string& label, const sf::Color& bg) {
        sf::RectangleShape box(sf::Vector2f(rect.width, rect.height));
        box.setPosition(rect.left, rect.top);
        box.setFillColor(bg);
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(12, 12, 12, 190));
        window.draw(box);
        drawText(label, rect.left + 110.f, rect.top + 22.f, 30, sf::Color::White);
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
        sf::RectangleShape topBar(sf::Vector2f(1280.f, 170.f));
        topBar.setPosition(0.f, 0.f);
        topBar.setFillColor(sf::Color(24, 30, 42, 235));
        window.draw(topBar);

        if (currentEnemyIndex < 0 || currentEnemyIndex >= static_cast<int>(enemySequence.size())) {
            drawText("敌人数据异常", 20.f, 78.f, 22, sf::Color(255, 120, 120));
            return;
        }
        const EnemyDef& def = enemySequence[currentEnemyIndex];

        drawTextureFit(logoTex, sf::FloatRect(18.f, 16.f, 34.f, 34.f));
        drawText("迷你尖塔：铁甲战士", 60.f, 12.f, 28, sf::Color::White);
        drawText("第 " + std::to_string(currentFloor + 1) + " / " + std::to_string(floorPlan.size()) + " 层", 20.f, 48.f, 22, sf::Color(220, 220, 220));
        drawText(def.name, 20.f, 78.f, 22, sf::Color(255, 220, 160));

                std::ostringstream p;
                p << "玩家生命 " << player.hp << "/" << player.maxHp << "  格挡 " << player.block << "  力量 " << player.strength
                    << "  虚弱 " << player.weak << "  易伤 " << player.vulnerable << "  能量 " << playerEnergy;
                drawText(p.str(), 20.f, 606.f, 20, sf::Color(200, 235, 200));

                drawText("牌组规模 " + std::to_string(masterDeck.size()) + "  |  金币 " + std::to_string(gold) + "  |  累计通关 " + std::to_string(totalWins),
                                 20.f, 636.f, 20, sf::Color(220, 220, 180));

        std::ostringstream e;
        e << "敌人生命 " << enemy.hp << "/" << enemy.maxHp << "  格挡 " << enemy.block << "  力量 " << enemy.strength << "  虚弱 "
          << enemy.weak << "  易伤 " << enemy.vulnerable;
        drawText(e.str(), 20.f, 140.f, 20, sf::Color(235, 200, 200));

        const Intent& intent = currentEnemyIntent();
        sf::RectangleShape intentBox(sf::Vector2f(470.f, 62.f));
        intentBox.setPosition(700.f, 66.f);
        intentBox.setFillColor(sf::Color(48, 40, 22, 230));
        intentBox.setOutlineThickness(2.f);
        intentBox.setOutlineColor(sf::Color(255, 210, 110, 230));
        window.draw(intentBox);
        drawTextureFit(intentTex, sf::FloatRect(712.f, 77.f, 44.f, 44.f));
        drawText("敌人意图：" + intent.label, 768.f, 82.f, 26, sf::Color(255, 235, 150));

        if (playerHitFlashTimer > 0.0f) {
            const float ratio = std::min(1.0f, playerHitFlashTimer / 0.22f);
            sf::RectangleShape flash(sf::Vector2f(1280.f, 170.f));
            flash.setPosition(0.f, 0.f);
            flash.setFillColor(sf::Color(220, 40, 40, static_cast<sf::Uint8>(35.f + ratio * 80.f)));
            window.draw(flash);
        }

        drawText("药剂 " + std::to_string(potionCount) + "/" + std::to_string(potionMax), potionRect.left + 14.f, potionRect.top + 10.f, 20, sf::Color(245, 235, 180));
        drawText("点击：+10格挡 抽1", potionRect.left + 14.f, potionRect.top + 38.f, 16, sf::Color(220, 220, 220));
    }

    void renderBattle() {
        renderStatusPanel();

        sf::RectangleShape enemyPanel(sf::Vector2f(360.f, 260.f));
        enemyPanel.setPosition(440.f, 185.f);
        enemyPanel.setFillColor(sf::Color(35, 43, 58, 220));
        enemyPanel.setOutlineThickness(2.f);
        enemyPanel.setOutlineColor(sf::Color(0, 0, 0, 170));
        window.draw(enemyPanel);
        drawTextureFit(*enemyPortrait(), sf::FloatRect(520.f, 205.f, 200.f, 200.f));

        if (pendingEnemy.active && !pendingEnemy.resolved) {
            sf::RectangleShape windup(sf::Vector2f(210.f, 38.f));
            windup.setPosition(515.f, 176.f);
            windup.setFillColor(sf::Color(150, 90, 40, 210));
            windup.setOutlineThickness(2.f);
            windup.setOutlineColor(sf::Color(0, 0, 0, 180));
            window.draw(windup);
            drawText("敌人蓄力中...", 542.f, 184.f, 20, sf::Color::White);
        }

        if (enemyHitFlashTimer > 0.0f) {
            const float ratio = std::min(1.0f, enemyHitFlashTimer / 0.22f);
            sf::RectangleShape enemyFlash(sf::Vector2f(360.f, 260.f));
            enemyFlash.setPosition(440.f, 185.f);
            enemyFlash.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(30.f + ratio * 120.f)));
            window.draw(enemyFlash);
        }

        sf::RectangleShape endTurn(sf::Vector2f(endTurnRect.width, endTurnRect.height));
        endTurn.setPosition(endTurnRect.left, endTurnRect.top);
        endTurn.setFillColor(sf::Color(45, 130, 70));
        endTurn.setOutlineThickness(2.f);
        endTurn.setOutlineColor(sf::Color::Black);
        window.draw(endTurn);
        drawText("结束回合", endTurnRect.left + 20.f, endTurnRect.top + 21.f, 26, sf::Color::White);

        sf::RectangleShape potionBtn(sf::Vector2f(potionRect.width, potionRect.height));
        potionBtn.setPosition(potionRect.left, potionRect.top);
        potionBtn.setFillColor(potionCount > 0 ? sf::Color(98, 76, 138, 220) : sf::Color(76, 76, 76, 190));
        potionBtn.setOutlineThickness(2.f);
        potionBtn.setOutlineColor(sf::Color(15, 15, 15, 190));
        window.draw(potionBtn);

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
            card.setFillColor(cardColor(c->type));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c->name, rects[i].left + 6.f, rects[i].top + 8.f - hoverLift, 16, sf::Color::White);
            drawText("费 " + std::to_string(c->cost), rects[i].left + 6.f, rects[i].top + 30.f - hoverLift, 14, sf::Color::White);
            drawTextureFit(*cardIcon(c->type), sf::FloatRect(rects[i].left + 70.f, rects[i].top + 6.f - hoverLift, 34.f, 34.f));
            drawWrappedText(c->desc, rects[i].left + 6.f, rects[i].top + 56.f - hoverLift, 13, sf::Color(240, 240, 240), 8);
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
                card.setFillColor(cardColor(c->type));
                card.setOutlineThickness(2.f);
                card.setOutlineColor(sf::Color::Black);
                window.draw(card);

                drawText(c->name, x + 6.f, y + 8.f, 16, sf::Color::White);
                drawText("费 " + std::to_string(c->cost), x + 6.f, y + 30.f, 14, sf::Color::White);
            }
        }

        if (turnBannerTimer > 0.0f) {
            const float t = std::min(1.0f, turnBannerTimer / 1.1f);
            const sf::Uint8 alpha = static_cast<sf::Uint8>(60.f + t * 140.f);
            sf::RectangleShape banner(sf::Vector2f(220.f, 46.f));
            banner.setPosition(530.f, 174.f);
            banner.setFillColor(sf::Color(55, 118, 70, alpha));
            banner.setOutlineThickness(2.f);
            banner.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));
            window.draw(banner);
            drawText("你的回合", 592.f, 184.f, 24, sf::Color(255, 255, 255, alpha));
        }

        float logY = 185.f;
        drawText("战斗日志", 20.f, logY, 20, sf::Color(220, 220, 220));
        logY += 28.f;
        for (const auto& line : logs) {
            drawWrappedText(line, 20.f, logY, 12, sf::Color(215, 215, 215), 26);
            logY += 16.f;
        }

        for (const auto& item : floatingTexts) {
            const float t = std::min(1.0f, item.remaining / item.duration);
            sf::Color c = item.color;
            c.a = static_cast<sf::Uint8>(70.f + t * 185.f);
            drawSfText(item.text, item.position.x, item.position.y, 20, c);
        }

        drawText(std::string("T：动画") + (enableCombatAnimations ? "开" : "关"), 180.f, 680.f, 18, sf::Color(180, 180, 180));
        drawText("R：重新开始本局", 20.f, 680.f, 18, sf::Color(180, 180, 180));
    }

    void renderReward() {
        drawText("选择 1 张奖励卡牌", 450.f, 80.f, 40, sf::Color(255, 255, 210));
        drawText("第 " + std::to_string(currentFloor + 1) + " 层战斗已完成", 500.f, 130.f, 24, sf::Color(220, 220, 220));

        auto rects = rewardRects();
        for (size_t i = 0; i < rewardChoices.size() && i < rects.size(); ++i) {
            const CardDef* c = findCard(rewardChoices[i]);
            if (!c) {
                continue;
            }

            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top);
            card.setFillColor(cardColor(c->type));
            card.setOutlineThickness(3.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c->name, rects[i].left + 10.f, rects[i].top + 12.f, 22, sf::Color::White);
            drawText("费用：" + std::to_string(c->cost), rects[i].left + 10.f, rects[i].top + 48.f, 18, sf::Color::White);
            drawTextureFit(*cardIcon(c->type), sf::FloatRect(rects[i].left + 166.f, rects[i].top + 10.f, 42.f, 42.f));
            drawWrappedText(c->desc, rects[i].left + 10.f, rects[i].top + 82.f, 16, sf::Color(240, 240, 240), 11);
        }

        drawText("点击任意一张卡牌继续", 500.f, 640.f, 24, sf::Color(230, 230, 230));
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

            drawText(c->name, rects[i].left + 10.f, rects[i].top + 8.f, 22, sf::Color::White);
            drawText("费 " + std::to_string(c->cost), rects[i].left + 10.f, rects[i].top + 40.f, 18, sf::Color(235, 235, 235));
            const int price = cardPrice(c->id);
            const sf::Color priceColor = (gold >= price) ? sf::Color(255, 220, 150) : sf::Color(255, 140, 140);
            drawText("价格 " + std::to_string(price), rects[i].left + 10.f, rects[i].top + 66.f, 18, priceColor);
            drawWrappedText(c->desc, rects[i].left + 10.f, rects[i].top + 94.f, 14, sf::Color(220, 220, 220), 11);
        }

        drawButton(shopLeaveRect, "离开商店（下一层）", sf::Color(72, 98, 126, 240));
        drawButton(shopPotionRect, "提升药剂上限（" + std::to_string(potionCapacityPrice) + "金币）", sf::Color(92, 76, 136, 240));
        drawButton(shopRemoveRect, "移除1张卡（" + std::to_string(deckRemovePrice) + "金币）", sf::Color(138, 82, 72, 240));
        drawText("红色价格代表金币不足", 940.f, 380.f, 20, sf::Color(255, 160, 160));
    }

    void renderEvent() {
        if (currentEventId == 1) {
            drawText("事件：生锈刀刃", 450.f, 80.f, 54, sf::Color(250, 210, 170));
            drawWrappedText("你在前方看见了一座荒废神殿。祭坛上有一把生锈匕首，似乎在渴望鲜血。", 170.f, 180.f, 28, sf::Color(235, 235, 235), 28);
            drawButton(eventLeftRect, "失去10生命，获得 邪恶之刃", sf::Color(130, 72, 72, 240));
            drawButton(eventRightRect, "无视匕首，拿走100金币", sf::Color(92, 118, 72, 240));
            drawText("选项影响：左-10生命，右+100金币", 410.f, 650.f, 22, sf::Color(230, 220, 190));
            return;
        }

        drawText("事件：救与不救", 470.f, 80.f, 54, sf::Color(250, 210, 170));
        drawWrappedText("一位看上去需要帮助的老人挡在路中央。你该如何选择？", 220.f, 190.f, 30, sf::Color(235, 235, 235), 24);
        drawButton(eventLeftRect, "施以援手，损失12生命", sf::Color(130, 72, 72, 240));
        drawButton(eventRightRect, "绕路找到剪刀，移除2张卡", sf::Color(92, 118, 72, 240));
        drawText("选项影响：左-12生命，右移除2张卡", 408.f, 650.f, 22, sf::Color(230, 220, 190));
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
            card.setFillColor(c ? cardColor(c->type) : sf::Color(90, 90, 90));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(sf::Color(0, 0, 0, 190));
            window.draw(card);

            if (!c) {
                continue;
            }
            drawText(c->name, rects[i].left + 6.f, rects[i].top + 6.f, 18, sf::Color::White);
            drawText("费 " + std::to_string(c->cost), rects[i].left + 6.f, rects[i].top + 32.f, 14, sf::Color(230, 230, 230));
            drawWrappedText(c->desc, rects[i].left + 6.f, rects[i].top + 50.f, 12, sf::Color(230, 230, 230), 16);
        }

        drawButton(deckDoneRect, "完成", sf::Color(76, 110, 140, 240));
    }

    void renderMainMenu() {
        drawTextureFit(logoTex, sf::FloatRect(550.f, 90.f, 180.f, 180.f));
        drawText("迷你尖塔 Demo", 480.f, 70.f, 52, sf::Color(255, 245, 230));
        drawText("铁甲战士的十五层试炼", 500.f, 255.f, 30, sf::Color(230, 230, 210));

        drawButton(menuStartRect, "开始游戏", sf::Color(58, 135, 86, 235));
        drawButton(menuExitRect, "退出游戏", sf::Color(146, 72, 72, 235));
        drawTextureFit(startTex, sf::FloatRect(menuStartRect.left + 30.f, menuStartRect.top + 22.f, 38.f, 38.f));
        drawTextureFit(exitTex, sf::FloatRect(menuExitRect.left + 30.f, menuExitRect.top + 22.f, 38.f, 38.f));

        drawText("点击按钮进行选择", 520.f, 540.f, 24, sf::Color(210, 210, 220));
        drawText("累计通关：" + std::to_string(totalWins), 20.f, 680.f, 20, sf::Color(220, 220, 180));
    }

    void renderDefeat() {
        drawText("战败", 560.f, 200.f, 84, sf::Color(240, 120, 120));
        drawText("本次挑战未能击败 BOSS", 430.f, 320.f, 34, sf::Color(230, 230, 230));
        drawButton(backMenuRect, "返回主菜单", sf::Color(86, 102, 145, 240));
    }

    void renderVictoryThanks() {
        drawTextureFit(thanksTex, sf::FloatRect(545.f, 170.f, 190.f, 190.f));
        drawText("感谢游玩", 510.f, 110.f, 64, sf::Color(125, 240, 170));
        drawText("你已击败 BOSS，正在进入制作人名单...", 340.f, 410.f, 30, sf::Color(236, 236, 236));
        drawText("点击可跳过等待", 530.f, 470.f, 24, sf::Color(210, 210, 220));
    }

    void renderCredits() {
        drawText("制作人名单", 510.f, 90.f, 62, sf::Color(250, 230, 170));
        drawText("[占位] 策划：待补充", 430.f, 230.f, 34, sf::Color(235, 235, 235));
        drawText("[占位] 程序：待补充", 430.f, 290.f, 34, sf::Color(235, 235, 235));
        drawText("[占位] 美术：待补充", 430.f, 350.f, 34, sf::Color(235, 235, 235));
        drawText("[占位] 音频：待补充", 430.f, 410.f, 34, sf::Color(235, 235, 235));
        drawButton(backMenuRect, "返回主菜单", sf::Color(86, 102, 145, 240));
    }

    void render() {
        window.clear(sf::Color(16, 20, 30));

        sf::VertexArray bg(sf::Quads, 4);
        bg[0].position = sf::Vector2f(0.f, 0.f);
        bg[1].position = sf::Vector2f(1280.f, 0.f);
        bg[2].position = sf::Vector2f(1280.f, 720.f);
        bg[3].position = sf::Vector2f(0.f, 720.f);
        bg[0].color = sf::Color(30, 36, 56);
        bg[1].color = sf::Color(42, 32, 54);
        bg[2].color = sf::Color(20, 24, 38);
        bg[3].color = sf::Color(17, 21, 33);
        window.draw(bg);

        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(std::clock()) * 0.0018f);
        sf::CircleShape glowA(220.f);
        glowA.setPosition(920.f, -90.f);
        glowA.setFillColor(sf::Color(255, 185, 90, static_cast<sf::Uint8>(22 + pulse * 26.f)));
        window.draw(glowA);

        sf::CircleShape glowB(280.f);
        glowB.setPosition(-120.f, 420.f);
        glowB.setFillColor(sf::Color(90, 160, 255, static_cast<sf::Uint8>(16 + pulse * 20.f)));
        window.draw(glowB);

        if (phase == Phase::MainMenu) {
            renderMainMenu();
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
