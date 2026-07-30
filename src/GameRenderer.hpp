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
    void render(const Game& game, Shader& shader, unsigned int cubeVAO);
    bool isValid() const { return hud.isValid(); }
private:
    Hud hud;

    void renderGrid(const Grid& grid, Shader& shader, unsigned int cubeVAO);
    void renderPlayer(const Player& player, Shader& shader, unsigned int cubeVAO);
    void renderEnemy(const Enemy& enemy, Shader& shader, unsigned int cubeVAO);
    void renderTargetBeam(const Enemy& enemy, Shader& shader, unsigned int cubeVAO);
};