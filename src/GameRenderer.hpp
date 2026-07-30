#pragma once

#include "Hud.hpp"

#include <glm/glm.hpp>

#include <vector>

class Game;
class Grid;
class Shader;
class Player;
class Enemy;

class GameRenderer
{
public:
    GameRenderer(int width, int height);
    void render(const Game& game, const Shader& shader, unsigned int cubeVAO, float visualTime);
    bool isValid() const
    {
        return hud.isValid();
    }

private:
    struct Material
    {
        glm::vec3 color;
        glm::vec3 emissionColor;
        float emissionStrength;
        float alpha;
    };

    struct CubeCommand
    {
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
        Material material;
    };

    Hud hud;

    unsigned int cachedShaderId{0};
    int modelLocation{-1};
    int objectColorLocation{-1};
    int emissionColorLocation{-1};
    int emissionStrengthLocation{-1};
    int objectAlphaLocation{-1};
    std::vector<CubeCommand> glowCommands;

    void drawCube(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                  const glm::vec3& rotation = glm::vec3(0.0f));
    void queueGlowCube(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                       const glm::vec3& rotation = glm::vec3(0.0f));
    void renderGlowPass();
    void renderGrid(const Grid& grid, float visualTime);
    void renderPlayer(const Player& player, float visualTime, bool poweredUp);
    void renderEnemy(const Enemy& enemy, float gameplayTimer, float visualTime);
    void cacheUniformLocations(const Shader& shader);
    void beginHudPass();
    void endHudPass();
};
