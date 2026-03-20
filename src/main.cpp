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
enum class Phase { MainMenu, Battle, Reward, VictoryThanks, Credits, Defeat };

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
    std::vector<EnemyDef> enemySequence;

    Phase phase = Phase::MainMenu;
    int battleIndex = 0;
    float phaseTimer = 0.0f;

    Combatant player;
    Combatant enemy;
    int enemyIntentIndex = 0;

    int playerEnergy = 3;
    int playerStrengthPerTurn = 0;
    int totalWins = 0;
    int potionCount = 1;

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

    float enemyHitFlashTimer = 0.0f;
    float playerHitFlashTimer = 0.0f;
    float turnBannerTimer = 0.0f;
    int hoveredCardIndex = -1;
    bool enableCombatAnimations = true;

    static constexpr const char* kSavePath = "save_progress.dat";
    static constexpr const char* kConfigPath = "game_config.ini";

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

        std::string line;
        while (std::getline(in, line)) {
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

            try {
                if (key == "max_fps") {
                    const int parsed = std::stoi(value);
                    config.maxFps = static_cast<unsigned int>(std::clamp(parsed, 30, 240));
                } else if (key == "enable_animations") {
                    config.enableAnimations = parseBool(value);
                } else if (key == "enable_floating_text") {
                    config.enableFloatingText = parseBool(value);
                } else if (key == "text_scale") {
                    const float parsed = std::stof(value);
                    config.textScale = std::clamp(parsed, 0.8f, 1.5f);
                }
            } catch (...) {
                std::cerr << "[warn] 配置项解析失败: " << key << "=" << value << '\n';
            }
        }
    }

    bool isValidTransition(Phase from, Phase to) const {
        if (from == to) {
            return true;
        }
        switch (from) {
            case Phase::MainMenu: return to == Phase::Battle;
            case Phase::Battle: return to == Phase::Reward || to == Phase::Defeat || to == Phase::VictoryThanks || to == Phase::MainMenu;
            case Phase::Reward: return to == Phase::Battle || to == Phase::MainMenu;
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
            {20, "恶魔形态", CardType::Power, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, true, "每回合开始获得 1 点力量，消耗"}
        };

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
            "精英敌人：装甲骑士", 78, true, false,
            {{"格挡 10 + 强化 +1 力量", 0, 1, 10, 1, 0, 0}, {"攻击 12", 12, 1, 0, 0, 0, 0}, {"攻击 7 x2", 7, 2, 0, 0, 0, 0}, {"施加易伤 1 + 攻击 8", 8, 1, 0, 0, 0, 1}}});

        enemySequence.push_back({
            "BOSS：尖塔守卫", 120, false, true,
            {{"强化 +2 力量", 0, 1, 0, 2, 0, 0}, {"攻击 14", 14, 1, 0, 0, 0, 0}, {"攻击 8 x2", 8, 2, 0, 0, 0, 0}, {"格挡 16 + 施加虚弱 1", 0, 1, 16, 0, 1, 0}, {"施加易伤 2 + 攻击 10", 10, 1, 0, 0, 0, 2}}});
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
        potionCount = 1;
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

        battleIndex = 0;
        phase = Phase::Battle;
        phaseTimer = 0.0f;
        startBattle();
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
        if (battleIndex < 0 || battleIndex >= static_cast<int>(enemySequence.size())) {
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

        const EnemyDef& def = enemySequence[battleIndex];
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

        if (cardId == 16) {
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
                pushLog(c->name + " 造成 " + std::to_string(dmg) + " 点伤害");
                if (enemy.hp <= 0) {
                    break;
                }
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
        potionCount = std::min(4, potionCount + 1);
        pushLog("获得 1 瓶药剂（当前 " + std::to_string(potionCount) + "）");
        if (battleIndex == static_cast<int>(enemySequence.size()) - 1) {
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

        const EnemyDef& def = enemySequence[battleIndex];
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

    const Intent& currentEnemyIntent() const {
        static const Intent fallback = fallbackIntent();
        if (battleIndex < 0 || battleIndex >= static_cast<int>(enemySequence.size())) {
            return fallback;
        }
        const EnemyDef& def = enemySequence[battleIndex];
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

        battleIndex++;
        transitionTo(Phase::Battle, "奖励选牌后进入下一战");
        startBattle();
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
        if (battleIndex < 0 || battleIndex >= static_cast<int>(enemySequence.size())) {
            return &enemy1Tex;
        }
        if (battleIndex == 0) return &enemy1Tex;
        if (battleIndex == 1) return &enemy2Tex;
        if (battleIndex == 2) return &enemy3Tex;
        if (battleIndex == 3) return &eliteTex;
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

    void renderStatusPanel() {
        sf::RectangleShape topBar(sf::Vector2f(1280.f, 170.f));
        topBar.setPosition(0.f, 0.f);
        topBar.setFillColor(sf::Color(24, 30, 42, 235));
        window.draw(topBar);

        if (battleIndex < 0 || battleIndex >= static_cast<int>(enemySequence.size())) {
            drawText("敌人数据异常", 20.f, 78.f, 22, sf::Color(255, 120, 120));
            return;
        }
        const EnemyDef& def = enemySequence[battleIndex];

        drawTextureFit(logoTex, sf::FloatRect(18.f, 16.f, 34.f, 34.f));
        drawText("迷你尖塔：铁甲战士", 60.f, 12.f, 28, sf::Color::White);
        drawText("第 " + std::to_string(battleIndex + 1) + " / 5 场战斗", 20.f, 48.f, 22, sf::Color(220, 220, 220));
        drawText(def.name, 20.f, 78.f, 22, sf::Color(255, 220, 160));

        std::ostringstream p;
        p << "玩家生命 " << player.hp << "/" << player.maxHp << "  格挡 " << player.block << "  力量 " << player.strength
          << "  虚弱 " << player.weak << "  易伤 " << player.vulnerable << "  能量 " << playerEnergy;
        drawText(p.str(), 20.f, 114.f, 20, sf::Color(200, 235, 200));

        drawText("牌组规模 " + std::to_string(masterDeck.size()) + "  |  累计通关 " + std::to_string(totalWins),
                 20.f, 640.f, 20, sf::Color(220, 220, 180));

        std::ostringstream e;
        e << "敌人生命 " << enemy.hp << "/" << enemy.maxHp << "  格挡 " << enemy.block << "  力量 " << enemy.strength << "  虚弱 "
          << enemy.weak << "  易伤 " << enemy.vulnerable;
        drawText(e.str(), 20.f, 140.f, 20, sf::Color(235, 200, 200));

        const Intent& intent = currentEnemyIntent();
        drawTextureFit(intentTex, sf::FloatRect(720.f, 78.f, 30.f, 30.f));
        drawText("意图：" + intent.label, 760.f, 78.f, 24, sf::Color(255, 235, 150));

        if (playerHitFlashTimer > 0.0f) {
            const float ratio = std::min(1.0f, playerHitFlashTimer / 0.22f);
            sf::RectangleShape flash(sf::Vector2f(1280.f, 170.f));
            flash.setPosition(0.f, 0.f);
            flash.setFillColor(sf::Color(220, 40, 40, static_cast<sf::Uint8>(35.f + ratio * 80.f)));
            window.draw(flash);
        }

        drawText("药剂 " + std::to_string(potionCount) + "/4", potionRect.left + 14.f, potionRect.top + 10.f, 20, sf::Color(245, 235, 180));
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
        drawText("战斗日志", 930.f, logY, 22, sf::Color(220, 220, 220));
        logY += 28.f;
        for (const auto& line : logs) {
            drawWrappedText(line, 930.f, logY, 14, sf::Color(215, 215, 215), 20);
            logY += 18.f;
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
        drawText("第 " + std::to_string(battleIndex + 1) + " 场战斗已完成", 500.f, 130.f, 24, sf::Color(220, 220, 220));

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

    void renderMainMenu() {
        drawTextureFit(logoTex, sf::FloatRect(550.f, 90.f, 180.f, 180.f));
        drawText("迷你尖塔 Demo", 480.f, 70.f, 52, sf::Color(255, 245, 230));
        drawText("铁甲战士的五场试炼", 500.f, 255.f, 30, sf::Color(230, 230, 210));

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
        } else if (phase == Phase::VictoryThanks) {
            renderVictoryThanks();
        } else if (phase == Phase::Credits) {
            renderCredits();
        } else if (phase == Phase::Defeat) {
            renderDefeat();
        }

        window.display();
    }
};

int main() {
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
