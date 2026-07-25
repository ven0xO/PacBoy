#pragma once

#include "Grid.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "GameState.hpp"
#include "Hud.hpp"
#include "Rect.hpp"

#include <memory>
#include <string>
#include <glm/glm.hpp>
//...

class Game
{
public:
    Game(const std::string& map_path, int screen_width, int screen_height);
    float getTimerOffset() const { return timerOffset; }
    Player* getPlayerPtr() const { return player.get(); }
    Grid* getGridPtr() { return &gameGrid; }

    void update(const float currentFrame);
    void render(Shader& shader, unsigned int cubeVAO);
    void nextLevel(float& lastFrame, const float currentFrame);

private:
    Grid gameGrid;
    GameState gameState;
    std::unique_ptr<Player> player;
    Hud hud;

    std::unique_ptr<Enemy> redEnemy;
    std::unique_ptr<Enemy> pinkEnemy;
    std::unique_ptr<Enemy> cyanEnemy;
    std::unique_ptr<Enemy> orangeEnemy;

    float invulnerableUntil{0.0f};
    float timerOffset{0.0f};
    std::string mapPath;

    const bool DEV{true};

    void checkEnemyCollision(Enemy* enemyPtr, Player* playerPtr, const float currentFrame);
};