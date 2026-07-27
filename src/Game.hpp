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
struct GLFWwindow;

enum class GamePhase
{
    MainMenu,
    Scoreboard,
    Ready,
    Playing,
    Paused,
    LevelComplete,
    GameOver
};

enum class MainMenuOption
{
    StartGame,
    Scoreboard,
    Count
};

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
    void processPlayerInput(GLFWwindow* window, const float currentFrame);

private:
    Grid gameGrid;
    GameState gameState;
    std::unique_ptr<Player> player;
    Hud hud;
    GamePhase phase{GamePhase::MainMenu};

    MainMenuOption selectedMenuOption{
        MainMenuOption::StartGame
    };

    std::unique_ptr<Enemy> redEnemy;
    std::unique_ptr<Enemy> pinkEnemy;
    std::unique_ptr<Enemy> cyanEnemy;
    std::unique_ptr<Enemy> orangeEnemy;

    float invulnerableUntil{0.0f};
    float timerOffset{0.0f};
    float readyTimer{0.0f};
    std::string mapPath;

    const bool DEV{true};

    void checkEnemyCollision(Enemy* enemyPtr, Player* playerPtr, const float currentFrame);

    float lastMoveTime{0.0f};
    static constexpr float MOVE_COOLDOWN{0.05f};

    bool prevKeyUp{false};
    bool prevKeyDown{false};
    bool prevKeyLeft{false};
    bool prevKeyRight{false};
    bool prevKeyEnter{false};
};
