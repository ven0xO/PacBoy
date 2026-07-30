#include "GameRenderer.hpp"

#include "Enemy.hpp"
#include "Game.hpp"
#include "Grid.hpp"
#include "Player.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include "../external/shader_s.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    constexpr float PI{3.14159265358979323846f};
    constexpr float DIRECTION_EPSILON{0.001f};

    bool isWall(const Grid& grid, int x, int y)
    {
        return x >= 0 && x < grid.getWidth() && y >= 0 && y < grid.getHeight() &&
               grid.getTile(x, y) == Tile::Wall;
    }

    glm::vec2 normalizedDirection(glm::vec2 direction, glm::vec2 fallback)
    {
        if (glm::length(direction) <= DIRECTION_EPSILON)
        {
            direction = fallback;
        }

        if (glm::length(direction) <= DIRECTION_EPSILON)
        {
            return glm::vec2(0.0f, 1.0f);
        }

        return glm::normalize(direction);
    }
} // namespace

GameRenderer::GameRenderer(int width, int height) : hud(width, height) {}

void GameRenderer::drawCube(const glm::vec3& position, const glm::vec3& scale,
                            const Material& material, const glm::vec3& rotation)
{
    glm::mat4 model{1.0f};
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(objectColorLocation, material.color.r, material.color.g, material.color.b);
    glUniform3f(emissionColorLocation, material.emissionColor.r, material.emissionColor.g,
                material.emissionColor.b);
    glUniform1f(emissionStrengthLocation, material.emissionStrength);
    glUniform1f(objectAlphaLocation, material.alpha);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void GameRenderer::queueGlowCube(const glm::vec3& position, const glm::vec3& scale,
                                 const Material& material, const glm::vec3& rotation)
{
    glowCommands.push_back({position, scale, rotation, material});
}

void GameRenderer::renderGlowPass()
{
    if (glowCommands.empty())
    {
        return;
    }

    GLint previousSourceRgb{GL_SRC_ALPHA};
    GLint previousDestinationRgb{GL_ONE_MINUS_SRC_ALPHA};
    GLint previousSourceAlpha{GL_ONE};
    GLint previousDestinationAlpha{GL_ONE_MINUS_SRC_ALPHA};
    GLboolean previousDepthMask{GL_TRUE};
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glGetIntegerv(GL_BLEND_SRC_RGB, &previousSourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousDestinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousDestinationAlpha);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (const CubeCommand& command : glowCommands)
    {
        drawCube(command.position, command.scale, command.material, command.rotation);
    }

    glDepthMask(previousDepthMask);
    glBlendFuncSeparate(
        static_cast<GLenum>(previousSourceRgb), static_cast<GLenum>(previousDestinationRgb),
        static_cast<GLenum>(previousSourceAlpha), static_cast<GLenum>(previousDestinationAlpha));

    if (blendWasEnabled == GL_FALSE)
    {
        glDisable(GL_BLEND);
    }
}

void GameRenderer::renderGrid(const Grid& grid, float visualTime)
{
    const Material floorEven{{0.012f, 0.018f, 0.045f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
    const Material floorOdd{{0.018f, 0.026f, 0.060f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
    const Material wall{{0.025f, 0.060f, 0.180f}, {0.0f, 0.055f, 0.22f}, 0.35f, 1.0f};
    const Material wallStrip{{0.020f, 0.36f, 1.0f}, {0.0f, 0.42f, 1.0f}, 2.2f, 1.0f};
    const Material wallHalo{{0.0f, 0.15f, 0.55f}, {0.0f, 0.42f, 1.0f}, 2.8f, 0.18f};
    const Material pellet{{1.0f, 0.88f, 0.45f}, {1.0f, 0.55f, 0.12f}, 0.55f, 1.0f};
    const Material energizer{{1.0f, 0.96f, 0.72f}, {0.55f, 0.25f, 1.0f}, 1.8f, 1.0f};
    const Material energizerHalo{{0.35f, 0.10f, 0.75f}, {0.48f, 0.18f, 1.0f}, 3.0f, 0.22f};
    const Material gate{{1.0f, 0.12f, 0.62f}, {1.0f, 0.04f, 0.42f}, 2.2f, 1.0f};
    const Material gateHalo{{0.65f, 0.03f, 0.32f}, {1.0f, 0.03f, 0.48f}, 3.0f, 0.18f};

    for (int y = 0; y < grid.getHeight(); y++)
    {
        for (int x = 0; x < grid.getWidth(); x++)
        {
            const Tile tile = grid.getTile(x, y);
            const glm::vec3 tilePosition{static_cast<float>(x), 0.0f, static_cast<float>(y)};

            if (tile == Tile::Wall)
            {
                drawCube(tilePosition + glm::vec3(0.0f, 0.46f, 0.0f),
                         glm::vec3(0.96f, 0.92f, 0.96f), wall);

                const glm::vec3 stripPosition = tilePosition + glm::vec3(0.0f, 0.94f, 0.0f);

                if (!isWall(grid, x, y - 1))
                {
                    drawCube(stripPosition + glm::vec3(0.0f, 0.0f, -0.42f),
                             glm::vec3(1.02f, 0.055f, 0.08f), wallStrip);
                    queueGlowCube(stripPosition + glm::vec3(0.0f, 0.015f, -0.42f),
                                  glm::vec3(1.08f, 0.13f, 0.18f), wallHalo);
                }

                if (!isWall(grid, x, y + 1))
                {
                    drawCube(stripPosition + glm::vec3(0.0f, 0.0f, 0.42f),
                             glm::vec3(1.02f, 0.055f, 0.08f), wallStrip);
                    queueGlowCube(stripPosition + glm::vec3(0.0f, 0.015f, 0.42f),
                                  glm::vec3(1.08f, 0.13f, 0.18f), wallHalo);
                }

                if (!isWall(grid, x - 1, y))
                {
                    drawCube(stripPosition + glm::vec3(-0.42f, 0.0f, 0.0f),
                             glm::vec3(0.08f, 0.055f, 1.02f), wallStrip);
                    queueGlowCube(stripPosition + glm::vec3(-0.42f, 0.015f, 0.0f),
                                  glm::vec3(0.18f, 0.13f, 1.08f), wallHalo);
                }

                if (!isWall(grid, x + 1, y))
                {
                    drawCube(stripPosition + glm::vec3(0.42f, 0.0f, 0.0f),
                             glm::vec3(0.08f, 0.055f, 1.02f), wallStrip);
                    queueGlowCube(stripPosition + glm::vec3(0.42f, 0.015f, 0.0f),
                                  glm::vec3(0.18f, 0.13f, 1.08f), wallHalo);
                }

                continue;
            }

            const Material& floorMaterial = (x + y) % 2 == 0 ? floorEven : floorOdd;
            drawCube(tilePosition + glm::vec3(0.0f, -0.075f, 0.0f), glm::vec3(0.96f, 0.08f, 0.96f),
                     floorMaterial);

            switch (tile)
            {
                case Tile::Pellet:
                    drawCube(tilePosition + glm::vec3(0.0f, 0.14f, 0.0f), glm::vec3(0.13f), pellet);
                    break;

                case Tile::Energizer:
                {
                    const float pulse = 1.0f + 0.14f * std::sin(visualTime * 5.0f);
                    const glm::vec3 rotation{PI / 4.0f, visualTime * 2.2f, PI / 4.0f};
                    const glm::vec3 position = tilePosition + glm::vec3(0.0f, 0.28f, 0.0f);

                    drawCube(position, glm::vec3(0.27f * pulse), energizer, rotation);
                    queueGlowCube(position, glm::vec3(0.46f * pulse), energizerHalo, rotation);
                    break;
                }

                case Tile::GhostSpawnEntrance:
                {
                    bool exitIsAlongX = false;

                    for (const glm::vec2& exit : grid.getGhostExitPositions())
                    {
                        const int deltaX = std::abs(static_cast<int>(exit.x) - x);
                        const int deltaY = std::abs(static_cast<int>(exit.y) - y);

                        if (deltaX + deltaY == 1)
                        {
                            exitIsAlongX = deltaX != 0;
                            break;
                        }
                    }

                    const glm::vec3 scale = exitIsAlongX ? glm::vec3(0.12f, 0.10f, 0.86f)
                                                         : glm::vec3(0.86f, 0.10f, 0.12f);
                    const glm::vec3 haloScale = exitIsAlongX ? glm::vec3(0.24f, 0.22f, 0.98f)
                                                             : glm::vec3(0.98f, 0.22f, 0.24f);
                    const glm::vec3 position = tilePosition + glm::vec3(0.0f, 0.38f, 0.0f);

                    drawCube(position, scale, gate);
                    queueGlowCube(position, haloScale, gateHalo);
                    break;
                }

                case Tile::Empty:
                case Tile::PacmanStart:
                case Tile::GhostStart:
                case Tile::GhostSpawnExit:
                case Tile::Tunnel:
                case Tile::Wall:
                    break;
            }
        }
    }
}

void GameRenderer::renderPlayer(const Player& player, float visualTime, bool poweredUp)
{
    const glm::vec2 position = player.getPosition();
    const glm::vec3 color = player.getColor();
    const float bob = std::sin(visualTime * 4.5f) * 0.025f;
    const float bodyY = 0.45f + bob;
    const float chomp = std::abs(std::sin(visualTime * 7.0f));
    const Material body{color, glm::vec3(1.0f, 0.55f, 0.02f), 0.28f, 1.0f};
    const Material mouth{{0.008f, 0.010f, 0.018f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
    const Material aura{{0.55f, 0.28f, 0.0f}, {1.0f, 0.54f, 0.02f}, 2.8f, 0.16f};
    const glm::vec3 center{position.x, bodyY, position.y};

    drawCube(center, glm::vec3(0.62f, 0.54f, 0.62f), body);
    drawCube(center + glm::vec3(0.0f, 0.34f, 0.0f), glm::vec3(0.46f, 0.16f, 0.46f), body);
    drawCube(center - glm::vec3(0.0f, 0.34f, 0.0f), glm::vec3(0.46f, 0.14f, 0.46f), body);
    drawCube(center + glm::vec3(0.37f, 0.0f, 0.0f), glm::vec3(0.14f, 0.42f, 0.42f), body);
    drawCube(center - glm::vec3(0.37f, 0.0f, 0.0f), glm::vec3(0.14f, 0.42f, 0.42f), body);
    drawCube(center + glm::vec3(0.0f, 0.0f, 0.37f), glm::vec3(0.42f, 0.42f, 0.14f), body);
    drawCube(center - glm::vec3(0.0f, 0.0f, 0.37f), glm::vec3(0.42f, 0.42f, 0.14f), body);

    const glm::vec2 direction =
        normalizedDirection(player.getCurrentDirection(), player.getCameraDirection());
    const glm::vec3 mouthPosition{position.x + direction.x * 0.445f, bodyY + 0.015f,
                                  position.y + direction.y * 0.445f};
    const glm::vec3 mouthScale{std::abs(direction.x) * 0.07f + std::abs(direction.y) * 0.40f,
                               0.055f + chomp * 0.22f,
                               std::abs(direction.y) * 0.07f + std::abs(direction.x) * 0.40f};
    drawCube(mouthPosition, mouthScale, mouth);

    if (poweredUp)
    {
        const float auraPulse = 1.0f + 0.06f * std::sin(visualTime * 6.0f);
        queueGlowCube(center, glm::vec3(1.02f * auraPulse), aura);
    }
}

void GameRenderer::renderEnemy(const Enemy& enemy, float gameplayTimer, float visualTime)
{
    const glm::vec2 position = enemy.get_position();
    const State state = enemy.get_state();
    const float phase = position.x * 0.53f + position.y * 0.31f;
    const float bob = std::sin(visualTime * 3.5f + phase) * 0.035f;
    const glm::vec3 center{position.x, 0.48f + bob, position.y};
    glm::vec3 bodyColor = enemy.getColor();

    if (state == State::Scared)
    {
        const float scaredTimeRemaining = enemy.getScaredTimeRemaining(gameplayTimer);
        const bool flashWhite = scaredTimeRemaining <= 2.0f &&
                                static_cast<int>(std::floor(gameplayTimer * 8.0f)) % 2 == 0;
        bodyColor = flashWhite ? glm::vec3(0.95f) : glm::vec3(0.04f, 0.16f, 0.82f);
    }

    if (state != State::Dead)
    {
        const float emissionStrength = state == State::Scared ? 0.45f : 0.16f;
        const Material body{bodyColor, bodyColor, emissionStrength, 1.0f};
        const Material aura{bodyColor * 0.45f, bodyColor, 1.5f, 0.075f};

        drawCube(center, glm::vec3(0.70f, 0.50f, 0.68f), body);
        drawCube(center + glm::vec3(0.0f, 0.34f, 0.0f), glm::vec3(0.52f, 0.18f, 0.52f), body);

        constexpr float FOOT_OFFSET_X{0.21f};
        constexpr float FOOT_OFFSET_Z{0.18f};
        constexpr float FOOT_HEIGHT{0.14f};

        drawCube(
            glm::vec3(position.x - FOOT_OFFSET_X, FOOT_HEIGHT + bob, position.y - FOOT_OFFSET_Z),
            glm::vec3(0.27f, 0.18f, 0.30f), body);
        drawCube(
            glm::vec3(position.x + FOOT_OFFSET_X, FOOT_HEIGHT + bob, position.y - FOOT_OFFSET_Z),
            glm::vec3(0.27f, 0.18f, 0.30f), body);
        drawCube(
            glm::vec3(position.x - FOOT_OFFSET_X, FOOT_HEIGHT + bob, position.y + FOOT_OFFSET_Z),
            glm::vec3(0.27f, 0.18f, 0.30f), body);
        drawCube(
            glm::vec3(position.x + FOOT_OFFSET_X, FOOT_HEIGHT + bob, position.y + FOOT_OFFSET_Z),
            glm::vec3(0.27f, 0.18f, 0.30f), body);

        queueGlowCube(center, glm::vec3(0.86f, 0.82f, 0.84f), aura);
    }

    const glm::vec2 direction = normalizedDirection(enemy.getDirection(), glm::vec2(0.0f, 1.0f));
    const glm::vec2 tangent{-direction.y, direction.x};
    const glm::vec3 front{direction.x, 0.0f, direction.y};
    const glm::vec3 side{tangent.x, 0.0f, tangent.y};
    const glm::vec3 eyeScale{std::abs(direction.x) * 0.07f + std::abs(direction.y) * 0.16f, 0.21f,
                             std::abs(direction.y) * 0.07f + std::abs(direction.x) * 0.16f};
    const glm::vec3 pupilScale{std::abs(direction.x) * 0.035f + std::abs(direction.y) * 0.075f,
                               0.085f,
                               std::abs(direction.y) * 0.035f + std::abs(direction.x) * 0.075f};
    const float eyeY = state == State::Dead ? 0.50f + bob : 0.58f + bob;
    const glm::vec3 faceCenter{position.x, eyeY, position.y};
    const Material eye{{0.96f, 0.98f, 1.0f}, {0.55f, 0.72f, 1.0f}, 0.35f, 1.0f};
    const Material pupil{{0.015f, 0.08f, 0.30f}, {0.02f, 0.12f, 0.55f}, 0.40f, 1.0f};

    for (const float sideOffset : {-0.17f, 0.17f})
    {
        const glm::vec3 eyePosition = faceCenter + front * 0.355f + side * sideOffset;
        drawCube(eyePosition, eyeScale, eye);
        drawCube(eyePosition + front * 0.055f, pupilScale, pupil);
    }
}

void GameRenderer::cacheUniformLocations(const Shader& shader)
{
    if (cachedShaderId == shader.ID)
    {
        return;
    }

    modelLocation = glGetUniformLocation(shader.ID, "model");
    objectColorLocation = glGetUniformLocation(shader.ID, "objectColor");
    emissionColorLocation = glGetUniformLocation(shader.ID, "emissionColor");
    emissionStrengthLocation = glGetUniformLocation(shader.ID, "emissionStrength");
    objectAlphaLocation = glGetUniformLocation(shader.ID, "objectAlpha");
    cachedShaderId = shader.ID;
}

void GameRenderer::beginHudPass()
{
    glDisable(GL_DEPTH_TEST);
    hud.beginRender();
}

void GameRenderer::endHudPass()
{
    hud.endRender();
    glEnable(GL_DEPTH_TEST);
}

void GameRenderer::render(const Game& game, const Shader& shader, unsigned int cubeVAO,
                          float visualTime)
{
    cacheUniformLocations(shader);
    glowCommands.clear();
    const GamePhase phase = game.getPhase();

    if (phase == GamePhase::MainMenu)
    {
        beginHudPass();
        hud.renderMainMenu(static_cast<int>(game.getSelectedMenuOption()));
        endHudPass();
        return;
    }

    if (phase == GamePhase::Scoreboard)
    {
        beginHudPass();
        hud.renderScoreboard(game.getScoreboard());
        endHudPass();
        return;
    }

    glBindVertexArray(cubeVAO);

    renderGrid(game.getGrid(), visualTime);

    const auto enemies = game.getEnemies();
    bool poweredUp = game.getPlayer().getEnergizer();

    for (const auto& enemy : enemies)
    {
        poweredUp = poweredUp || enemy.get().get_state() == State::Scared;
    }

    renderPlayer(game.getPlayer(), visualTime, poweredUp);

    const float gameplayTimer = game.getGameplayTimer();
    for (const auto& enemy : enemies)
    {
        renderEnemy(enemy.get(), gameplayTimer, visualTime);
    }

    renderGlowPass();
    glBindVertexArray(0);

    beginHudPass();

    const GameState& state = game.getGameState();
    hud.render(state);

    if (phase == GamePhase::Ready)
    {
        hud.renderReady();
    }
    else if (phase == GamePhase::Paused)
    {
        hud.renderPause(static_cast<int>(game.getSelectedPauseMenuOption()));
    }
    else if (phase == GamePhase::LevelComplete)
    {
        hud.renderLevelComplete(state);
    }
    else if (phase == GamePhase::GameOver)
    {
        hud.renderGameOver(state);
    }
    else if (phase == GamePhase::NewScore)
    {
        const int score = state.getScore();
        hud.renderHighScore(game.getEnteredName(), score,
                            game.getScoreboard().getHighScore() < score);
    }

    endHudPass();
}
