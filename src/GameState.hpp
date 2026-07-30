#pragma once

class GameState
{
public:
    void addScore(int points);
    void loseLife();
    void nextLevel();
    void collectEnergizer();
    void collectPellet();
    bool checkIfNextLevel() const;

    int getScore() const
    {
        return score;
    }
    int getLives() const
    {
        return lives;
    }
    int getLevel() const
    {
        return level;
    }
    bool isGameOver() const
    {
        return gameOver;
    }
    void resetState();

    void setPelletCount(int pCount)
    {
        pelletsCount = pCount;
    }
    void setEnergizerCount(int eCount)
    {
        energizerCount = eCount;
    }

private:
    int score{0};
    int lives{3};
    int level{1};
    bool gameOver{false};
    int pelletsCount{0};
    int energizerCount{0};
};
