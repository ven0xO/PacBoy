#include "Game.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

Game::Game(const std::string& map_path, int screen_width, int screen_height) 
            : hud(screen_width, screen_height),
            mapPath(map_path)
{
    if (!hud.isValid() || !gameGrid.loadFromFile(map_path))
    {
        return;
    }

    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    glm::vec2 pacmanStart = gameGrid.getPacmanStartPosition();
    player = std::make_unique<Player>(
        pacmanStart.x,
        pacmanStart.y,
        &gameState
    );

    redEnemy = std::make_unique<Enemy>(
        Type::Red,
        &gameGrid,
        player.get(),
        gameGrid.getRedGhostSpawnPosition(),
        &gameState
    );
    pinkEnemy = std::make_unique<Enemy>(
        Type::Pink,
        &gameGrid,
        player.get(),
        gameGrid.getPinkGhostSpawnPosition(),
        &gameState
    );
    cyanEnemy = std::make_unique<Enemy>(
        Type::Blue,
        &gameGrid,
        player.get(),
        gameGrid.getBlueGhostSpawnPosition(),
        &gameState
    );
    orangeEnemy = std::make_unique<Enemy>(
        Type::Orange,
        &gameGrid,
        player.get(),
        gameGrid.getOrangeGhostSpawnPosition(),
        &gameState
    );

    pinkEnemy->set_red_ghost(redEnemy.get());
    cyanEnemy->set_red_ghost(redEnemy.get());
    orangeEnemy->set_red_ghost(redEnemy.get());

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
    
    int level = gameState.getLevel();

    redEnemy->update(gameplayTimer, level, deltaTime);
    pinkEnemy->update(gameplayTimer, level, deltaTime);
    cyanEnemy->update(gameplayTimer, level, deltaTime);
    orangeEnemy->update(gameplayTimer, level, deltaTime);

    if(player->getEnergizer()) player->resetEnergizer();

    Rect playerRect = player->getPlayerRect();

    bool red_collision = redEnemy->checkCollision(playerRect);
    bool pink_collision = pinkEnemy->checkCollision(playerRect);
    bool cyan_collision = cyanEnemy->checkCollision(playerRect);
    bool orange_collision = orangeEnemy->checkCollision(playerRect);

    if(red_collision) checkEnemyCollision(redEnemy.get(), player.get(), currentFrame);
    if(pink_collision) checkEnemyCollision(pinkEnemy.get(), player.get(), currentFrame);
    if(cyan_collision) checkEnemyCollision(cyanEnemy.get(), player.get(), currentFrame);
    if(orange_collision) checkEnemyCollision(orangeEnemy.get(), player.get(), currentFrame);
}

void Game::render(Shader& shader, unsigned int cubeVAO)
{

    if (phase == GamePhase::MainMenu)
    {
        glDisable(GL_DEPTH_TEST);
        hud.renderMainMenu(static_cast<int>(selectedMenuOption));
        glEnable(GL_DEPTH_TEST);
        return;
    }

    if (phase == GamePhase::Scoreboard)
    {
        glDisable(GL_DEPTH_TEST);
        hud.renderScoreboard(scoreboard);
        glEnable(GL_DEPTH_TEST);
        return;
    }

    gameGrid.render(shader, cubeVAO);
    player->render(shader, cubeVAO);

    if(DEV)
    {
        redEnemy->renderTargetBeam(shader, cubeVAO);
        pinkEnemy->renderTargetBeam(shader, cubeVAO);
        cyanEnemy->renderTargetBeam(shader, cubeVAO);
        orangeEnemy->renderTargetBeam(shader, cubeVAO);
    }

    redEnemy->render(shader, cubeVAO);
    pinkEnemy->render(shader, cubeVAO);
    cyanEnemy->render(shader, cubeVAO);
    orangeEnemy->render(shader, cubeVAO);

    glDisable(GL_DEPTH_TEST);
    hud.render(gameState);
    if(phase == GamePhase::Ready)
    {
        hud.renderReady();
    }
    else if(phase == GamePhase::Paused)
    {
        hud.renderPause(static_cast<int>(selectedPauseMenuOption));
    }
    else if(phase == GamePhase::GameOver)
    {
        hud.renderGameOver(gameState);
    }
    else if(phase == GamePhase::NewScore)
    {
        int score = gameState.getScore();
        hud.renderHighScore(enteredName, score, scoreboard.getHighScore() < score);
    }
    glEnable(GL_DEPTH_TEST);
}

void Game::nextLevel()
{
    if(phase != GamePhase::Playing) return;

    
    if(gameState.checkIfNextLevel())
    {
        if(gameGrid.loadFromFile(mapPath))
        {
            gameState.setPelletCount(gameGrid.getInitPelletCount());
            gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

            player->resetPlayer();
            redEnemy->resetGhost();
            pinkEnemy->resetGhost();
            cyanEnemy->resetGhost();
            orangeEnemy->resetGhost();

            gameplayTimer = 0.0f;
            invulnerableUntil = 0.0f;
        }
    }
}

void Game::checkEnemyCollision(Enemy* enemyPtr, Player* playerPtr, const float currentFrame)
{
    State currentState = enemyPtr->get_state();

    if(currentState == State::Scared)
    {
        enemyPtr->set_state_dead();
        playerPtr->killGhost();
    }
    else if(currentState != State::Dead && currentFrame >= invulnerableUntil)
    {
        gameState.loseLife();

        if(!gameState.isGameOver())
        {
            resetRound(currentFrame);
        }
        else
        {
            phase = GamePhase::GameOver;
        }
    }
}

void Game::processPlayerInput(GLFWwindow* window, const float currentFrame)
{
    //if(phase != GamePhase::Playing) return;
    const bool updateCamera = phase == GamePhase::Ready;

    bool keyUp = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool keyDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool keyLeft = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool keyRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    bool keyEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    bool keyBackspace =
        glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    bool keyP = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

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

        for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; key++)
        {
            const std::size_t index =
                static_cast<std::size_t>(key - GLFW_KEY_A);

            const bool keyPressed =
                glfwGetKey(window, key) == GLFW_PRESS;

            if (
                keyPressed &&
                !prevLetterKeys[index] &&
                enteredName.size() < maxNameLength
            )
            {
                enteredName.push_back(
                    static_cast<char>('A' + index)
                );
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
    if (!gameGrid.loadFromFile(mapPath))
    {
        std::cerr << "Failed to start a new game.\n";
        return false;
    }

    gameState.resetState();
    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    player->resetPlayer();
    redEnemy->resetGhost();
    pinkEnemy->resetGhost();
    cyanEnemy->resetGhost();
    orangeEnemy->resetGhost();

    gameplayTimer = 0.0f;
    readyTimer = currentFrame + 2.0f;
    invulnerableUntil = 0.0f;
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

bool Game::resetRound(float currentFrame)
{
    player->resetPlayer();
    redEnemy->resetGhost();
    pinkEnemy->resetGhost();
    cyanEnemy->resetGhost();
    orangeEnemy->resetGhost();

    gameplayTimer = 0.0f;
    readyTimer = currentFrame + 2.0f;
    invulnerableUntil = currentFrame + 1.5f;

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
