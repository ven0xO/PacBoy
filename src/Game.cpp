#include "Game.hpp"
#include <algorithm>
#include <iostream>

namespace
{
    constexpr float READY_DURATION{2.0f};
    constexpr float LEVEL_COMPLETE_DURATION{2.5f};
    constexpr float HIT_INVULNERABILITY_DURATION{1.5f};

    constexpr std::array<Type, 4> ENEMY_TYPES{
        Type::Red,
        Type::Pink,
        Type::Blue,
        Type::Orange
    };

    std::array<glm::vec2, 4> getEnemySpawnPositions(
        const Grid& grid
    )
    {
        return {
            grid.getRedGhostSpawnPosition(),
            grid.getPinkGhostSpawnPosition(),
            grid.getBlueGhostSpawnPosition(),
            grid.getOrangeGhostSpawnPosition()
        };
    }
}

Game::Game(const std::vector<std::string>& level_paths) : levelPaths(level_paths)
{
    if (levelPaths.empty())
    {
        std::cerr << "No level files configured.\n";
        return;
    }

    if (!gameGrid.loadFromFile(levelPaths.front()))
    {
        return;
    }

    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    glm::vec2 pacmanStart = gameGrid.getPacmanStartPosition();
    player = std::make_unique<Player>(
        pacmanStart.x,
        pacmanStart.y,
        gameState
    );

    const auto spawnPositions =
        getEnemySpawnPositions(gameGrid);

    for (std::size_t index = 0; index < enemies.size(); ++index)
    {
        enemies[index] = std::make_unique<Enemy>(
            ENEMY_TYPES[index],
            gameGrid,
            *player,
            spawnPositions[index]
        );

        if (ENEMY_TYPES[index] == Type::Blue)
        {
            enemies[index]->set_red_ghost(*enemies[0]);
        }
    }

    initialized = true;
}

void Game::update(float currentFrame, float deltaTime)
{
    if (phase == GamePhase::Ready &&
        readyTimer > 0.0f &&
        currentFrame >= readyTimer)
    {
        phase = GamePhase::Playing;
        readyTimer = 0.0f;
    }

    if(phase != GamePhase::Playing) return;

    gameplayTimer += deltaTime;

    player->update(gameGrid, deltaTime);

    if (player->getEnergizer())
    {
        for (const auto& enemy : enemies)
        {
            enemy->enterScared(gameplayTimer);
        }

        player->resetEnergizer();
    }
    
    int level = gameState.getLevel();

    for (const auto& enemy : enemies)
    {
        enemy->update(gameplayTimer, level, deltaTime);
    }

    handleEnemyCollisions(currentFrame);
}

void Game::nextLevel(float currentFrame)
{
    if (
        phase == GamePhase::Playing &&
        gameState.checkIfNextLevel()
    )
    {
        phase = GamePhase::LevelComplete;
        levelCompleteUntil =
            currentFrame + LEVEL_COMPLETE_DURATION;
        return;
    }

    if (
        phase == GamePhase::LevelComplete &&
        currentFrame >= levelCompleteUntil
    )
    {
        loadNextLevel(currentFrame);
    }
}

void Game::handleEnemyCollisions(float currentFrame)
{
    const Rect playerRect = player->getPlayerRect();

    const bool hitByDangerousGhost = std::any_of(
        enemies.begin(),
        enemies.end(),
        [&playerRect](const auto& enemy)
        {
            const State state = enemy->get_state();
            return enemy->checkCollision(playerRect) &&
                (state == State::Chase || state == State::Scatter);
        }
    );

    // A dangerous collision has priority and can remove at most one life.
    if (
        hitByDangerousGhost &&
        gameplayTimer >= invulnerableUntil
    )
    {
        gameState.loseLife();

        if (!gameState.isGameOver())
        {
            resetRound(currentFrame);
        }
        else
        {
            phase = GamePhase::GameOver;
        }

        return;
    }

    for (const auto& enemy : enemies)
    {
        if (
            enemy->get_state() == State::Scared &&
            enemy->checkCollision(playerRect)
        )
        {
            enemy->set_state_dead();
            player->killGhost();
        }
    }
}

void Game::processPlayerInput(const GameInput& input, float currentFrame)
{
    const bool updateCamera =
        phase == GamePhase::Ready;

    const bool keyUp = input.up;
    const bool keyDown = input.down;
    const bool keyLeft = input.left;
    const bool keyRight = input.right;
    const bool keyEnter = input.enter;
    const bool keyBackspace = input.backspace;
    const bool keyP = input.pause;

    if(keyP && !prevKeyP)
    {
        if(phase == GamePhase::Playing)
        {
            phase = GamePhase::Paused;
            selectedPauseMenuOption = PauseMenuOption::Resume;
        }
        else if(phase == GamePhase::Paused)
        {
            phase = GamePhase::Playing;
        }
    }

    if(phase == GamePhase::Playing || phase == GamePhase::Ready)
    {
        if (keyUp && !prevKeyUp)
        {
            player->setDirection(Direction::Forward, updateCamera);
        }
        else if (keyDown && !prevKeyDown)
        {
            player->setDirection(Direction::Back, updateCamera);
        }
        else if (keyLeft && !prevKeyLeft)
        {
            player->setDirection(Direction::Left, updateCamera);
        }
        else if (keyRight && !prevKeyRight)
        {
            player->setDirection(Direction::Right, updateCamera);
        }
    }
    else if(phase == GamePhase::MainMenu)
    {
        int selected = static_cast<int>(selectedMenuOption);

        const int optionCount = static_cast<int>(MainMenuOption::Count);

        if (keyUp && !prevKeyUp)
        {
            selected = std::max(0, selected - 1);
        }
        else if (keyDown && !prevKeyDown)
        {
            selected = std::min(optionCount - 1, selected + 1);
        }

        selectedMenuOption = static_cast<MainMenuOption>(selected);

        if(keyEnter && !prevKeyEnter)
        {
            if(selectedMenuOption == MainMenuOption::StartGame)
            {
                if (startNewGame(currentFrame))
                {
                    return;
                }
            }
            else if (selectedMenuOption == MainMenuOption::Scoreboard)
            {
                phase = GamePhase::Scoreboard;
            }
        }
    }
    else if(phase == GamePhase::Paused)
    {
        int selected = static_cast<int>(selectedPauseMenuOption);
        const int optionCount = static_cast<int>(PauseMenuOption::Count);

        if(keyUp && !prevKeyUp)
        {
            selected = std::max(0, selected - 1);
        }
        else if(keyDown && !prevKeyDown)
        {
            selected = std::min(optionCount - 1, selected + 1);
        }

        selectedPauseMenuOption =
            static_cast<PauseMenuOption>(selected);

        if(keyEnter && !prevKeyEnter)
        {
            if(selectedPauseMenuOption == PauseMenuOption::Resume)
            {
                phase = GamePhase::Playing;
            }
            else if(selectedPauseMenuOption == PauseMenuOption::MainMenu)
            {
                phase = GamePhase::MainMenu;
            }
        }
    }
    else if(phase == GamePhase::Scoreboard)
    {
        if(keyEnter && !prevKeyEnter)
        {
            phase = GamePhase::MainMenu;
        }
    }
    else if(phase == GamePhase::LevelComplete)
    {
        if(keyEnter && !prevKeyEnter)
        {
            loadNextLevel(currentFrame);
        }
    }
    else if(phase == GamePhase::GameOver)
    {
        if (keyEnter && !prevKeyEnter)
        {
            if (scoreboard.isHighScore(gameState.getScore()))
            {
                enteredName.clear();
                prevLetterKeys.fill(false);
                phase = GamePhase::NewScore;
            }
            else
            {
                phase = GamePhase::MainMenu;
            }
        }
    }
    else if(phase == GamePhase::NewScore)
    {
        constexpr std::size_t maxNameLength{10};

        for (std::size_t index = 0; index < input.letters.size(); ++index)
        {
            const bool keyPressed = input.letters[index];

            if (keyPressed && !prevLetterKeys[index] && enteredName.size() < maxNameLength)
            {
                enteredName.push_back(static_cast<char>('A' + static_cast<int>(index)));
            }

            prevLetterKeys[index] = keyPressed;
        }

        if (
            keyBackspace &&
            !prevKeyBackspace &&
            !enteredName.empty()
        )
        {
            enteredName.pop_back();
        }

        if (
            keyEnter &&
            !prevKeyEnter &&
            !enteredName.empty()
        )
        {
            scoreboard.addScore(
                enteredName,
                gameState.getScore()
            );

            phase = GamePhase::Scoreboard;
        }
    }
    

    prevKeyUp = keyUp;
    prevKeyDown = keyDown;
    prevKeyLeft = keyLeft;
    prevKeyRight = keyRight;
    prevKeyEnter = keyEnter;
    prevKeyBackspace = keyBackspace;
    prevKeyP = keyP;
}

bool Game::startNewGame(float currentFrame)
{
    if (
        levelPaths.empty() ||
        !gameGrid.loadFromFile(levelPaths.front())
    )
    {
        std::cerr << "Failed to start a new game.\n";
        return false;
    }

    currentLevelIndex = 0;
    gameState.resetState();
    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    resetEntitiesForLoadedLevel();

    gameplayTimer = 0.0f;
    readyTimer = currentFrame + READY_DURATION;
    invulnerableUntil = 0.0f;
    levelCompleteUntil = 0.0f;
    enteredName.clear();

    selectedMenuOption = MainMenuOption::StartGame;
    selectedPauseMenuOption = PauseMenuOption::Resume;

    prevKeyUp = false;
    prevKeyDown = false;
    prevKeyLeft = false;
    prevKeyRight = false;
    prevKeyEnter = false;
    prevKeyBackspace = false;
    prevKeyP = false;
    prevLetterKeys.fill(false);

    phase = GamePhase::Ready;
    return true;
}

bool Game::loadNextLevel(float currentFrame)
{
    // Reaching the end starts the configured level list again.
    const std::size_t nextLevelIndex =
        (currentLevelIndex + 1) % levelPaths.size();

    if (!gameGrid.loadFromFile(levelPaths[nextLevelIndex]))
    {
        std::cerr << "Failed to load the next level.\n";
        levelCompleteUntil =
            currentFrame + LEVEL_COMPLETE_DURATION;
        return false;
    }

    currentLevelIndex = nextLevelIndex;
    gameState.nextLevel();
    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    resetEntitiesForLoadedLevel();

    gameplayTimer = 0.0f;
    readyTimer = currentFrame + READY_DURATION;
    invulnerableUntil = 0.0f;
    levelCompleteUntil = 0.0f;
    phase = GamePhase::Ready;
    return true;
}

void Game::resetEntitiesForLoadedLevel()
{
    player->resetPlayer(gameGrid.getPacmanStartPosition());

    const auto spawnPositions =
        getEnemySpawnPositions(gameGrid);

    for (std::size_t index = 0; index < enemies.size(); ++index)
    {
        enemies[index]->resetGhost(spawnPositions[index]);
    }
}

bool Game::resetRound(float currentFrame)
{
    player->resetPlayer();

    for (const auto& enemy : enemies)
    {
        enemy->resetGhost();
    }

    gameplayTimer = 0.0f;
    readyTimer = currentFrame + READY_DURATION;
    invulnerableUntil = HIT_INVULNERABILITY_DURATION;
    levelCompleteUntil = 0.0f;

    selectedMenuOption = MainMenuOption::StartGame;
    selectedPauseMenuOption = PauseMenuOption::Resume;

    prevKeyUp = false;
    prevKeyDown = false;
    prevKeyLeft = false;
    prevKeyRight = false;
    prevKeyEnter = false;
    prevKeyBackspace = false;
    prevKeyP = false;
    prevLetterKeys.fill(false);

    phase = GamePhase::Ready;
    return true;
}
