#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <vector>

enum class CardType {
    Attack,
    Skill,
    Power
};

enum class Phase {
    Battle,
    Reward,
    Victory,
    Defeat
};

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

class Game {
public:
    Game()
        : window(sf::VideoMode(1280, 720), "MiniSpireSFML"),
          rng(static_cast<std::mt19937::result_type>(std::random_device{}())) {
        window.setFramerateLimit(60);
        loadFont();
                loadTextures();
        initCards();
        initEnemies();
                loadProgress();
        resetRun();
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                handleEvent(event);
            }

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

    std::mt19937 rng;

    std::vector<CardDef> cardPool;
    std::unordered_map<int, CardDef> cardsById;
    std::vector<EnemyDef> enemySequence;

    Phase phase = Phase::Battle;
    int battleIndex = 0;

    Combatant player;
    Combatant enemy;
    int enemyIntentIndex = 0;

    int playerEnergy = 3;
    int playerStrengthPerTurn = 0;
    int totalWins = 0;

    std::vector<int> masterDeck;
    std::vector<int> drawPile;
    std::vector<int> discardPile;
    std::vector<int> hand;
    std::vector<int> exhaustPile;

    std::vector<int> rewardChoices;

    std::deque<std::string> logs;

    sf::FloatRect endTurnRect{1070.f, 620.f, 170.f, 70.f};

    static constexpr const char* kSavePath = "save_progress.dat";

    void loadFont() {
        hasFont = font.loadFromFile("assets/fonts/NotoSansCJKsc-Regular.otf") ||
                  font.loadFromFile("assets/font.ttf") ||
                  font.loadFromFile("C:/Windows/Fonts/arial.ttf");
    }

    void loadTexture(sf::Texture& tex, const std::string& path) {
        tex.loadFromFile(path);
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
    }

    void pushLog(const std::string& line) {
        logs.push_back(line);
        while (logs.size() > 12) {
            logs.pop_front();
        }
    }

    void initCards() {
        cardPool = {
            {1, "Strike", CardType::Attack, 1, 6, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "Deal 6 damage."},
            {2, "Defend", CardType::Skill, 1, 0, 1, 5, 0, 0, 0, 0, 0, 0, false, false, "Gain 5 Block."},
            {3, "Bash", CardType::Attack, 2, 8, 1, 0, 0, 2, 0, 0, 0, 0, false, false, "Deal 8 damage. Apply 2 Vulnerable."},
            {4, "Heavy Slash", CardType::Attack, 2, 14, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "Deal 14 damage."},
            {5, "Shield Up", CardType::Skill, 1, 0, 1, 8, 0, 0, 0, 0, 0, 0, false, false, "Gain 8 Block."},
            {6, "Rage", CardType::Skill, 1, 0, 1, 0, 2, 0, 0, 0, 0, 0, false, false, "Gain 2 Strength."},
            {7, "Pommel Blow", CardType::Attack, 1, 9, 1, 0, 0, 0, 0, 1, 0, 0, false, false, "Deal 9 damage. Draw 1 card."},
            {8, "True Grit Lite", CardType::Skill, 1, 0, 1, 9, 0, 0, 0, 0, 0, 0, true, false, "Gain 9 Block. Exhaust."},
            {9, "Cleave", CardType::Attack, 1, 7, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "Deal 7 damage."},
            {10, "Battle Trance", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, true, false, "Draw 2 cards. Exhaust."},
            {11, "Iron Wave", CardType::Attack, 1, 5, 1, 5, 0, 0, 0, 0, 0, 0, false, false, "Deal 5 damage. Gain 5 Block."},
            {12, "Power Through", CardType::Skill, 1, 0, 1, 14, 0, 0, 0, 0, 0, 0, false, false, "Gain 14 Block."},
            {13, "Shrug It Off", CardType::Skill, 1, 0, 1, 8, 0, 0, 0, 1, 0, 0, false, false, "Gain 8 Block. Draw 1 card."},
            {14, "Uppercut", CardType::Attack, 2, 11, 1, 0, 0, 1, 1, 0, 0, 0, false, false, "Deal 11 damage. Apply 1 Weak and 1 Vulnerable."},
            {15, "Sword Rain", CardType::Attack, 1, 4, 3, 0, 0, 0, 0, 0, 0, 0, false, false, "Deal 4 damage 3 times."},
            {16, "Limit Break Lite", CardType::Skill, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, false, "Double your Strength. Exhaust."},
            {17, "Rampage", CardType::Attack, 1, 10, 1, 0, 0, 0, 0, 0, 0, 0, false, false, "Deal 10 damage."},
            {18, "Offering Lite", CardType::Skill, 0, 0, 1, 0, 0, 0, 0, 3, 1, 6, true, false, "Lose 6 HP. Gain 1 Energy. Draw 3 cards. Exhaust."},
            {19, "Impervious", CardType::Skill, 2, 0, 1, 30, 0, 0, 0, 0, 0, 0, true, false, "Gain 30 Block. Exhaust."},
            {20, "Demon Form Lite", CardType::Power, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, true, true, "At the start of each turn, gain 1 Strength. Exhaust."}
        };

        for (const auto& c : cardPool) {
            cardsById[c.id] = c;
        }
    }

    void initEnemies() {
        enemySequence.clear();

        enemySequence.push_back({
            "Normal: Slime Bruiser", 45, false, false,
            {
                {"Attack 7", 7, 1, 0, 0, 0, 0},
                {"Block 6 + Attack 5", 5, 1, 6, 0, 0, 0},
                {"Attack 9", 9, 1, 0, 0, 0, 0}
            }
        });

        enemySequence.push_back({
            "Normal: Cult Adept", 52, false, false,
            {
                {"Buff +2 STR", 0, 1, 0, 2, 0, 0},
                {"Attack 6 x2", 6, 2, 0, 0, 0, 0},
                {"Attack 10", 10, 1, 0, 0, 0, 0}
            }
        });

        enemySequence.push_back({
            "Normal: Louse Raider", 58, false, false,
            {
                {"Apply Weak 1 + Attack 6", 6, 1, 0, 0, 1, 0},
                {"Attack 8", 8, 1, 0, 0, 0, 0},
                {"Block 8", 0, 1, 8, 0, 0, 0}
            }
        });

        enemySequence.push_back({
            "Elite: Armored Knight", 95, true, false,
            {
                {"Block 10 + Buff +1 STR", 0, 1, 10, 1, 0, 0},
                {"Attack 12", 12, 1, 0, 0, 0, 0},
                {"Attack 7 x2", 7, 2, 0, 0, 0, 0},
                {"Apply Vulnerable 1 + Attack 8", 8, 1, 0, 0, 0, 1}
            }
        });

        enemySequence.push_back({
            "BOSS: Spire Warden", 150, false, true,
            {
                {"Buff +2 STR", 0, 1, 0, 2, 0, 0},
                {"Attack 14", 14, 1, 0, 0, 0, 0},
                {"Attack 8 x2", 8, 2, 0, 0, 0, 0},
                {"Block 16 + Weak 1", 0, 1, 16, 0, 1, 0},
                {"Apply Vulnerable 2 + Attack 10", 10, 1, 0, 0, 0, 2}
            }
        });
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

    void resetRun() {
        player.maxHp = 80;
        player.hp = 80;
        player.block = 0;
        player.strength = 0;
        player.weak = 0;
        player.vulnerable = 0;

        playerStrengthPerTurn = 0;

        drawPile.clear();
        discardPile.clear();
        hand.clear();
        exhaustPile.clear();
        rewardChoices.clear();
        logs.clear();
        masterDeck.clear();

        // Start deck: 5 Strike, 4 Defend, 1 Bash
        for (int i = 0; i < 5; ++i) masterDeck.push_back(1);
        for (int i = 0; i < 4; ++i) masterDeck.push_back(2);
        masterDeck.push_back(3);

        battleIndex = 0;
        phase = Phase::Battle;
        startBattle();
    }

    void shuffle(std::vector<int>& pile) {
        std::shuffle(pile.begin(), pile.end(), rng);
    }

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
        drawPile = masterDeck;
        shuffle(drawPile);
        hand.clear();
        discardPile.clear();
        exhaustPile.clear();

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

        pushLog("Battle start: " + def.name);
        startPlayerTurn();
    }

    void startPlayerTurn() {
        player.block = 0;
        playerEnergy = 3;
        player.strength += playerStrengthPerTurn;
        drawCards(5);
        pushLog("Player turn started.");
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

    void dealDamage(Combatant& target, int damage) {
        if (damage <= 0) {
            return;
        }
        const int blocked = std::min(target.block, damage);
        target.block -= blocked;
        target.hp -= (damage - blocked);
        if (target.hp < 0) {
            target.hp = 0;
        }
    }

    void applyCard(int handIndex) {
        if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
            return;
        }

        const int cardId = hand[handIndex];
        const CardDef& c = cardsById[cardId];

        if (playerEnergy < c.cost) {
            pushLog("Not enough energy for " + c.name + ".");
            return;
        }

        playerEnergy -= c.cost;

        if (c.name == "Limit Break Lite") {
            player.strength *= 2;
        }

        if (c.grantsStrengthPerTurn) {
            playerStrengthPerTurn += 1;
            pushLog("Power active: +1 Strength each turn.");
        }

        if (c.damage > 0 && enemy.hp > 0) {
            for (int i = 0; i < c.hits; ++i) {
                const int dmg = adjustedDamage(c.damage, player, enemy);
                dealDamage(enemy, dmg);
                pushLog(c.name + " deals " + std::to_string(dmg) + " damage.");
                if (enemy.hp <= 0) {
                    break;
                }
            }
        }

        if (c.block > 0) {
            player.block += c.block;
            pushLog(c.name + " grants " + std::to_string(c.block) + " Block.");
        }

        if (c.gainStrength > 0) {
            player.strength += c.gainStrength;
            pushLog(c.name + " grants " + std::to_string(c.gainStrength) + " Strength.");
        }

        if (c.applyVulnerable > 0 && enemy.hp > 0) {
            enemy.vulnerable += c.applyVulnerable;
            pushLog("Enemy gains " + std::to_string(c.applyVulnerable) + " Vulnerable.");
        }

        if (c.applyWeak > 0 && enemy.hp > 0) {
            enemy.weak += c.applyWeak;
            pushLog("Enemy gains " + std::to_string(c.applyWeak) + " Weak.");
        }

        if (c.gainEnergy > 0) {
            playerEnergy += c.gainEnergy;
            pushLog(c.name + " gains " + std::to_string(c.gainEnergy) + " Energy.");
        }

        if (c.loseHp > 0) {
            player.hp = std::max(0, player.hp - c.loseHp);
            pushLog(c.name + " costs " + std::to_string(c.loseHp) + " HP.");
        }

        if (c.draw > 0) {
            drawCards(c.draw);
            pushLog(c.name + " draws " + std::to_string(c.draw) + " card(s).");
        }

        hand.erase(hand.begin() + handIndex);
        if (c.exhaust) {
            exhaustPile.push_back(cardId);
        } else {
            discardPile.push_back(cardId);
        }

        if (player.hp <= 0) {
            phase = Phase::Defeat;
            pushLog("You are defeated.");
            return;
        }

        if (enemy.hp <= 0) {
            onBattleWon();
        }
    }

    void onBattleWon() {
        pushLog("Battle won!");
        if (battleIndex == static_cast<int>(enemySequence.size()) - 1) {
            totalWins++;
            saveProgress();
            phase = Phase::Victory;
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
        phase = Phase::Reward;
    }

    void endPlayerTurn() {
        for (int id : hand) {
            discardPile.push_back(id);
        }
        hand.clear();

        if (player.weak > 0) --player.weak;
        if (player.vulnerable > 0) --player.vulnerable;

        executeEnemyTurn();
    }

    void executeEnemyTurn() {
        if (phase != Phase::Battle) {
            return;
        }

        enemy.block = 0;

        const EnemyDef& def = enemySequence[battleIndex];
        const Intent& intent = def.pattern[enemyIntentIndex % def.pattern.size()];
        enemyIntentIndex++;

        if (intent.block > 0) {
            enemy.block += intent.block;
            pushLog("Enemy gains " + std::to_string(intent.block) + " Block.");
        }

        if (intent.gainStrength > 0) {
            enemy.strength += intent.gainStrength;
            pushLog("Enemy gains " + std::to_string(intent.gainStrength) + " Strength.");
        }

        if (intent.applyWeak > 0) {
            player.weak += intent.applyWeak;
            pushLog("Player gains " + std::to_string(intent.applyWeak) + " Weak.");
        }

        if (intent.applyVulnerable > 0) {
            player.vulnerable += intent.applyVulnerable;
            pushLog("Player gains " + std::to_string(intent.applyVulnerable) + " Vulnerable.");
        }

        if (intent.damage > 0) {
            for (int i = 0; i < intent.hits; ++i) {
                const int dmg = adjustedDamage(intent.damage, enemy, player);
                dealDamage(player, dmg);
                pushLog("Enemy deals " + std::to_string(dmg) + " damage.");
                if (player.hp <= 0) {
                    break;
                }
            }
        }

        if (enemy.weak > 0) --enemy.weak;
        if (enemy.vulnerable > 0) --enemy.vulnerable;

        if (player.hp <= 0) {
            phase = Phase::Defeat;
            pushLog("You are defeated.");
            return;
        }

        startPlayerTurn();
    }

    std::vector<sf::FloatRect> handRects() const {
        std::vector<sf::FloatRect> rects;
        const float cardW = 150.f;
        const float cardH = 210.f;
        const float gap = 12.f;
        const float totalW = static_cast<float>(hand.size()) * cardW +
                             static_cast<float>(std::max(0, static_cast<int>(hand.size()) - 1)) * gap;
        float startX = (1280.f - totalW) * 0.5f;
        const float y = 480.f;
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
        const EnemyDef& def = enemySequence[battleIndex];
        return def.pattern[enemyIntentIndex % def.pattern.size()];
    }

    void pickReward(int index) {
        if (index < 0 || index >= static_cast<int>(rewardChoices.size())) {
            return;
        }
        const int picked = rewardChoices[index];
        masterDeck.push_back(picked);
        pushLog("Card added to deck: " + cardsById[picked].name);

        battleIndex++;
        phase = Phase::Battle;
        startBattle();
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
            resetRun();
            return;
        }

        if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) {
            return;
        }

        const sf::Vector2f mouse(
            static_cast<float>(event.mouseButton.x),
            static_cast<float>(event.mouseButton.y));

        if (phase == Phase::Battle) {
            if (endTurnRect.contains(mouse)) {
                endPlayerTurn();
                return;
            }

            auto rects = handRects();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (rects[i].contains(mouse)) {
                    applyCard(static_cast<int>(i));
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
        }
    }

    void drawText(const std::string& text, float x, float y, unsigned int size, sf::Color color) {
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

    void renderStatusPanel() {
        sf::RectangleShape topBar(sf::Vector2f(1280.f, 170.f));
        topBar.setPosition(0.f, 0.f);
        topBar.setFillColor(sf::Color(24, 30, 42, 235));
        window.draw(topBar);

        const EnemyDef& def = enemySequence[battleIndex];

        drawTextureFit(logoTex, sf::FloatRect(18.f, 16.f, 34.f, 34.f));
        drawText("MiniSpire SFML - Ironclad", 60.f, 12.f, 28, sf::Color::White);
        drawText("Battle " + std::to_string(battleIndex + 1) + "/5", 20.f, 48.f, 22, sf::Color(220, 220, 220));
        drawText(def.name, 20.f, 78.f, 22, sf::Color(255, 220, 160));

        std::ostringstream p;
        p << "Player HP " << player.hp << "/" << player.maxHp
          << "  Block " << player.block
          << "  STR " << player.strength
          << "  Weak " << player.weak
          << "  Vuln " << player.vulnerable
          << "  Energy " << playerEnergy;
        drawText(p.str(), 20.f, 114.f, 20, sf::Color(200, 235, 200));

        drawText("Deck " + std::to_string(masterDeck.size()) + " cards  |  Total Wins " + std::to_string(totalWins),
             20.f, 640.f, 20, sf::Color(220, 220, 180));

        std::ostringstream e;
        e << "Enemy HP " << enemy.hp << "/" << enemy.maxHp
          << "  Block " << enemy.block
          << "  STR " << enemy.strength
          << "  Weak " << enemy.weak
          << "  Vuln " << enemy.vulnerable;
        drawText(e.str(), 20.f, 140.f, 20, sf::Color(235, 200, 200));

        const Intent& intent = currentEnemyIntent();
        std::ostringstream in;
        in << "Intent: " << intent.label;
        drawTextureFit(intentTex, sf::FloatRect(720.f, 78.f, 30.f, 30.f));
        drawText(in.str(), 760.f, 78.f, 24, sf::Color(255, 235, 150));
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

        sf::RectangleShape endTurn(sf::Vector2f(endTurnRect.width, endTurnRect.height));
        endTurn.setPosition(endTurnRect.left, endTurnRect.top);
        endTurn.setFillColor(sf::Color(45, 130, 70));
        endTurn.setOutlineThickness(2.f);
        endTurn.setOutlineColor(sf::Color::Black);
        window.draw(endTurn);
        drawText("End Turn", endTurnRect.left + 26.f, endTurnRect.top + 21.f, 26, sf::Color::White);

        auto rects = handRects();
        for (size_t i = 0; i < hand.size(); ++i) {
            const CardDef& c = cardsById[hand[i]];
            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top);
            card.setFillColor(cardColor(c.type));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c.name, rects[i].left + 8.f, rects[i].top + 8.f, 18, sf::Color::White);
            drawText("Cost: " + std::to_string(c.cost), rects[i].left + 8.f, rects[i].top + 34.f, 16, sf::Color::White);
            drawTextureFit(*cardIcon(c.type), sf::FloatRect(rects[i].left + 96.f, rects[i].top + 8.f, 46.f, 46.f));
            drawText(c.desc, rects[i].left + 8.f, rects[i].top + 62.f, 14, sf::Color(240, 240, 240));
        }

        float logY = 185.f;
        drawText("Battle Log", 930.f, logY, 22, sf::Color(220, 220, 220));
        logY += 28.f;
        for (const auto& line : logs) {
            drawText(line, 930.f, logY, 14, sf::Color(215, 215, 215));
            logY += 18.f;
        }

        drawText("R: Restart Run", 20.f, 680.f, 18, sf::Color(180, 180, 180));
    }

    void renderReward() {
        drawText("Choose 1 card reward", 470.f, 80.f, 40, sf::Color(255, 255, 210));
        drawText("Battle " + std::to_string(battleIndex + 1) + " cleared", 540.f, 130.f, 24, sf::Color(220, 220, 220));

        auto rects = rewardRects();
        for (size_t i = 0; i < rewardChoices.size() && i < rects.size(); ++i) {
            const CardDef& c = cardsById[rewardChoices[i]];

            sf::RectangleShape card(sf::Vector2f(rects[i].width, rects[i].height));
            card.setPosition(rects[i].left, rects[i].top);
            card.setFillColor(cardColor(c.type));
            card.setOutlineThickness(3.f);
            card.setOutlineColor(sf::Color::Black);
            window.draw(card);

            drawText(c.name, rects[i].left + 10.f, rects[i].top + 12.f, 22, sf::Color::White);
            drawText("Cost: " + std::to_string(c.cost), rects[i].left + 10.f, rects[i].top + 48.f, 18, sf::Color::White);
            drawTextureFit(*cardIcon(c.type), sf::FloatRect(rects[i].left + 166.f, rects[i].top + 10.f, 42.f, 42.f));
            drawText(c.desc, rects[i].left + 10.f, rects[i].top + 82.f, 16, sf::Color(240, 240, 240));
        }

        drawText("Click one card to continue", 500.f, 640.f, 24, sf::Color(230, 230, 230));
    }

    void renderEndScreen(bool won) {
        const std::string title = won ? "Victory" : "Defeat";
        const sf::Color color = won ? sf::Color(120, 230, 150) : sf::Color(240, 120, 120);

        drawText(title, 560.f, 240.f, 70, color);
        drawText("You completed 5 battles and defeated the boss." , 350.f, 340.f, 28, sf::Color(230, 230, 230));
        if (!won) {
            drawText("Run ended before boss victory.", 470.f, 380.f, 24, sf::Color(230, 230, 230));
        }
        drawText("Press R to restart", 500.f, 480.f, 30, sf::Color(220, 220, 220));
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

        if (phase == Phase::Battle) {
            renderBattle();
        } else if (phase == Phase::Reward) {
            renderReward();
        } else if (phase == Phase::Victory) {
            renderEndScreen(true);
        } else if (phase == Phase::Defeat) {
            renderEndScreen(false);
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
