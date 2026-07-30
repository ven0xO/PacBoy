#include "GameRenderer.hpp"

#include "Grid.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Game.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include "../external/shader_s.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GameRenderer::GameRenderer(int width, int height) : hud(width, height) {}

void GameRenderer::renderGrid(const Grid& grid)
{
    for (int y{}; y < grid.getHeight(); y++)
    {
        for (int x = 0; x < grid.getWidth(); x++)
        {
            Tile tile = grid.getTile(x, y);
            switch (tile)
            {
                case Tile::Empty:
                case Tile::Tunnel:
                case Tile::GhostStart:
                case Tile::GhostSpawnExit:
                case Tile::PacmanStart:
                    continue;

                case Tile::Wall:
                    glUniform3f(objectColorLocation, 0.0f, 0.0f, 1.0f);
                    break;

                case Tile::Pellet:
                    glUniform3f(objectColorLocation, 1.0f, 1.0f, 0.0f);
                    break;

                case Tile::Energizer:
                    glUniform3f(objectColorLocation, 1.0f, 0.0f, 1.0f);
                    break;

                case Tile::GhostSpawnEntrance:
                    glUniform3f(objectColorLocation, 1.0f, 0.0f, 0.0f);
                    break;
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, 0.0f, y));

            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }
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

void GameRenderer::renderPlayer(const Player& player)
{
    const glm::vec2 position = player.getPosition();
    const glm::vec3& color = player.getColor();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, 0.1f, position.y));
    model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(objectColorLocation, color.r, color.g, color.b);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void GameRenderer::renderEnemy(const Enemy& enemy)
{
    const glm::vec2 position = enemy.get_position();
    const glm::vec3& color = enemy.getColor();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, 0.1f, position.y));
    model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(objectColorLocation, color.r, color.g, color.b);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void GameRenderer::render(const Game& game, const Shader& shader, unsigned int cubeVAO)
{
    cacheUniformLocations(shader);
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

    renderGrid(game.getGrid());

    renderPlayer(game.getPlayer());

    const auto enemies = game.getEnemies();

    for (const auto& enemy : enemies)
    {
        renderEnemy(enemy.get());
    }

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
