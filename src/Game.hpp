#pragma once

#include "Grid.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "GameEvent.hpp"
#include "GameState.hpp"
#include "Scoreboard.hpp"
#include "GameInput.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
    Game(const std::vector<std::string>& level_paths);

    bool isInitialized() const
    {
        return initialized;
    }

    void update(float currentFrame, float deltaTime);
    void nextLevel(float currentFrame);
    void processPlayerInput(const GameInput& input, float currentFrame);
    bool startNewGame(float currentFrame);
    bool resetRound(float currentFrame);
    std::vector<GameEvent> takeEvents();

    const Grid& getGrid() const
    {
        return gameGrid;
    }
    const Player& getPlayer() const
    {
        return *player;
    }
    std::array<std::reference_wrapper<const Enemy>, 4> getEnemies() const
    {
        return {std::cref(*enemies[0]), std::cref(*enemies[1]), std::cref(*enemies[2]),
                std::cref(*enemies[3])};
    }
    const GameState& getGameState() const
    {
        return gameState;
    }
    GamePhase getPhase() const
    {
        return phase;
    }
    MainMenuOption getSelectedMenuOption() const
    {
        return selectedMenuOption;
    }
    PauseMenuOption getSelectedPauseMenuOption() const
    {
        return selectedPauseMenuOption;
    }
    const Scoreboard& getScoreboard() const
    {
        return scoreboard;
    }
    const std::string& getEnteredName() const
    {
        return enteredName;
    }
    float getGameplayTimer() const
    {
        return gameplayTimer;
    }

private:
    Grid gameGrid;
    GameState gameState;
    std::unique_ptr<Player> player;
    Scoreboard scoreboard{"./assets/scores/scores.json"};
    GamePhase phase{GamePhase::MainMenu};

    MainMenuOption selectedMenuOption{MainMenuOption::StartGame};

    PauseMenuOption selectedPauseMenuOption{PauseMenuOption::Resume};

    std::array<std::unique_ptr<Enemy>, 4> enemies;

    float invulnerableUntil{0.0f};
    float readyTimer{0.0f};
    float gameplayTimer{0.0f};
    float levelCompleteUntil{0.0f};
    std::vector<std::string> levelPaths;
    std::size_t currentLevelIndex{0};
    std::string enteredName;
    std::vector<GameEvent> pendingEvents;
    bool initialized{false};

    void handleEnemyCollisions(float currentFrame);
    bool loadNextLevel(float currentFrame);
    void resetEntitiesForLoadedLevel();

    bool prevKeyUp{false};
    bool prevKeyDown{false};
    bool prevKeyLeft{false};
    bool prevKeyRight{false};
    bool prevKeyEnter{false};
    bool prevKeyBackspace{false};
    bool prevKeyP{false};
    std::array<bool, 26> prevLetterKeys{};
};
