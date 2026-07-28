#pragma once

#include "Grid.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "GameState.hpp"
#include "Hud.hpp"
#include "Rect.hpp"
#include "Scoreboard.hpp"

#include <array>
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
    GameOver,
    NewScore
};

enum class MainMenuOption
{
    StartGame,
    Scoreboard,
    Count
};

enum class PauseMenuOption
{
    Resume,
    MainMenu,
    Count
};

class Game
{
public:
    Game(const std::string& map_path, int screen_width, int screen_height);
    Player* getPlayerPtr() const { return player.get(); }
    Grid* getGridPtr() { return &gameGrid; }
    bool isInitialized() const { return initialized; }

    void update(float currentFrame, float deltaTime);
    void render(Shader& shader, unsigned int cubeVAO);
    void nextLevel();
    void processPlayerInput(GLFWwindow* window, const float currentFrame);
    bool startNewGame(float currentFrame);
    bool resetRound(float currentFrame);

private:
    Grid gameGrid;
    GameState gameState;
    std::unique_ptr<Player> player;
    Hud hud;
    Scoreboard scoreboard{"./assets/scores/scores.json"};
    GamePhase phase{GamePhase::MainMenu};

    MainMenuOption selectedMenuOption{
        MainMenuOption::StartGame
    };

    PauseMenuOption selectedPauseMenuOption{
        PauseMenuOption::Resume
    };

    std::unique_ptr<Enemy> redEnemy;
    std::unique_ptr<Enemy> pinkEnemy;
    std::unique_ptr<Enemy> cyanEnemy;
    std::unique_ptr<Enemy> orangeEnemy;

    float invulnerableUntil{0.0f};
    float readyTimer{0.0f};
    float gameplayTimer{0.0f};
    std::string mapPath;
    std::string enteredName;
    bool initialized{false};

    static constexpr bool DEV{false};

    void checkEnemyCollision(Enemy* enemyPtr, Player* playerPtr, const float currentFrame);

    bool prevKeyUp{false};
    bool prevKeyDown{false};
    bool prevKeyLeft{false};
    bool prevKeyRight{false};
    bool prevKeyEnter{false};
    bool prevKeyBackspace{false};
    bool prevKeyP{false};
    std::array<bool, 26> prevLetterKeys{};
};
