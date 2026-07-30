#pragma once

#include "Hud.hpp"

class Game;
class Grid;
class Shader;
class Player;
class Enemy;

class GameRenderer
{
public:
    GameRenderer(int width, int height);
    void render(const Game& game, const Shader& shader, unsigned int cubeVAO);
    bool isValid() const
    {
        return hud.isValid();
    }

private:
    Hud hud;

    unsigned int cachedShaderId{0};
    int modelLocation{-1};
    int objectColorLocation{-1};

    void renderGrid(const Grid& grid);
    void renderPlayer(const Player& player);
    void renderEnemy(const Enemy& enemy);
    void cacheUniformLocations(const Shader& shader);
    void beginHudPass();
    void endHudPass();
};
