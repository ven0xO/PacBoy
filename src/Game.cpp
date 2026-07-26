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
        hud.renderMainMenu();
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
    }
}

void Game::processPlayerInput(GLFWwindow* window)
{
    //if(phase != GamePhase::Playing) return;


    float currentTime = static_cast<float>(glfwGetTime());

    if (currentTime - lastMoveTime <= MOVE_COOLDOWN)
    {
        return;
    }

    bool keyUp = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool keyDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool keyLeft = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool keyRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    if(phase == GamePhase::Playing)
    {
        if (keyUp && !prevKeyUp)
        {
            player->setDirection(Direction::Forward);
            lastMoveTime = currentTime;
        }
        else if (keyDown && !prevKeyDown)
        {
            player->setDirection(Direction::Back);
            lastMoveTime = currentTime;
        }
        else if (keyLeft && !prevKeyLeft)
        {
            player->setDirection(Direction::Left);
            lastMoveTime = currentTime;
        }
        else if (keyRight && !prevKeyRight)
        {
            player->setDirection(Direction::Right);
            lastMoveTime = currentTime;
        }
    }
    else if(phase == GamePhase::MainMenu)
    {
        if (keyUp && !prevKeyUp)
        {
            hud.setSelected(std::max(0, hud.getSelected() - 1));
        }
        else if (keyDown && !prevKeyDown)
        {
            hud.setSelected(std::min(hud.getSelectedLen() - 1, hud.getSelected() + 1));
        }
    }
    

    prevKeyUp = keyUp;
    prevKeyDown = keyDown;
    prevKeyLeft = keyLeft;
    prevKeyRight = keyRight;
}
