#include "Game.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>

Game::Game(const std::string& map_path, int screen_width, int screen_height) 
            : hud(screen_width, screen_height),
            mapPath(map_path)
{
    bool success = gameGrid.loadFromFile(map_path);
    if(!success)
    {
        std::cerr << "Failed to load game grid from file." << std::endl;
    }

    gameState.setPelletCount(gameGrid.getInitPelletCount());
    gameState.setEnergizerCount(gameGrid.getInitEnergizerCount());

    glm::vec2 pacmanStart = gameGrid.getPacmanStartPosition();
    player = std::make_unique<Player>(
        pacmanStart.x,
        pacmanStart.y,
        &gameState
    );

    glm::vec2 ghostSpawn = gameGrid.getGhostSpawnPosition();
    redEnemy = std::make_unique<Enemy>(
        Type::Red,
        &gameGrid,
        player.get(),
        ghostSpawn,
        &gameState
    );
    pinkEnemy = std::make_unique<Enemy>(
        Type::Pink,
        &gameGrid,
        player.get(),
        ghostSpawn,
        &gameState
    );
    cyanEnemy = std::make_unique<Enemy>(
        Type::Blue,
        &gameGrid,
        player.get(),
        ghostSpawn,
        &gameState
    );
    orangeEnemy = std::make_unique<Enemy>(
        Type::Orange,
        &gameGrid,
        player.get(),
        ghostSpawn,
        &gameState
    );

    pinkEnemy->set_red_ghost(redEnemy.get());
    cyanEnemy->set_red_ghost(redEnemy.get());
    orangeEnemy->set_red_ghost(redEnemy.get());
}

void Game::update(const float currentFrame)
{
    if (phase == GamePhase::Ready &&
        readyTimer > 0.0f &&
        currentFrame >= readyTimer)
    {
        phase = GamePhase::Playing;
        readyTimer = 0.0f;
    }

    if(phase != GamePhase::Playing) return;

    player->update(gameGrid);
    
    int level = gameState.getLevel();

    redEnemy->update(currentFrame, level);
    pinkEnemy->update(currentFrame, level);
    cyanEnemy->update(currentFrame, level);
    orangeEnemy->update(currentFrame, level);

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
    glEnable(GL_DEPTH_TEST);
}

void Game::nextLevel(float& lastFrame, const float currentFrame)
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

            lastFrame = 0.0f;
            timerOffset += currentFrame;
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
        invulnerableUntil = currentFrame + 1.5f;

        if(!gameState.isGameOver())
        {
            playerPtr->setPosition(gameGrid.getPacmanStartPosition());
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


    float currentTime = static_cast<float>(glfwGetTime());

    if (currentTime - lastMoveTime <= MOVE_COOLDOWN)
    {
        return;
    }

    bool keyUp = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool keyDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool keyLeft = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool keyRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    bool keyEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
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
            lastMoveTime = currentTime;
        }
        else if (keyDown && !prevKeyDown)
        {
            player->setDirection(Direction::Back, updateCamera);
            lastMoveTime = currentTime;
        }
        else if (keyLeft && !prevKeyLeft)
        {
            player->setDirection(Direction::Left, updateCamera);
            lastMoveTime = currentTime;
        }
        else if (keyRight && !prevKeyRight)
        {
            player->setDirection(Direction::Right, updateCamera);
            lastMoveTime = currentTime;
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
                phase = GamePhase::Ready;
                readyTimer = currentFrame + 2.0f;
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
                phase = GamePhase::NewScore;
            }
            else
            {
                phase = GamePhase::MainMenu;
            }
        }
    }
    

    prevKeyUp = keyUp;
    prevKeyDown = keyDown;
    prevKeyLeft = keyLeft;
    prevKeyRight = keyRight;
    prevKeyEnter = keyEnter;
    prevKeyP = keyP;
}
