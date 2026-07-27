#include "GameState.hpp"

void GameState::addScore(int points)
{
    score += points;
}

void GameState::loseLife()
{
    if (lives > 0)
    {
        lives--;
    }

    if (lives == 0)
    {
        gameOver = true;
    }
}

void GameState::nextLevel()
{
    level++;
}

void GameState::collectPellet()
{
    pelletsCount--;
}

void GameState::collectEnergizer()
{
    energizerCount--;
}

bool GameState::checkIfNextLevel()
{
    if(!(pelletsCount == 0 && energizerCount == 0)) return false;

    level++;
    return true;
}

void GameState::resetState()
{
    score = 0;
    lives = 3;
    level = 1;
    gameOver = false;
}