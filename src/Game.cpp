#include "Game.hpp"

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