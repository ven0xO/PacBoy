#include "GameRenderer.hpp"

#include "Grid.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Game.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include "../external/shader_s.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GameRenderer::GameRenderer(int width, int height)
    : hud(width, height)
{

}

void GameRenderer::renderGrid(
    const Grid& grid,
    Shader& shader,
    unsigned int cubeVAO
)
{
    for(int y{}; y < grid.getHeight(); y++)
    {
        for(int x = 0; x < grid.getWidth(); x++)
        {
            Tile tile = grid.getTile(x, y);
            if (tile == Tile::Empty ||
                tile == Tile::Tunnel ||
                tile == Tile::GhostStart ||
                tile == Tile::GhostSpawnExit ||
                tile == Tile::PacmanStart)
            {
                continue;
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, 0.0f, y));

            int modelLoc = glGetUniformLocation(shader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
            switch (tile) {
                case Tile::Wall:
                    glUniform3f(objectColorLoc, 0.0f, 0.0f, 1.0f); // blue
                    break;
                case Tile::Pellet:
                    glUniform3f(objectColorLoc, 1.0f, 1.0f, 0.0f); // yellow
                    break;
                case Tile::Energizer:
                    glUniform3f(objectColorLoc, 1.0f, 0.0f, 1.0f); // purple
                    break;
                case Tile::GhostSpawnEntrance:
                    glUniform3f(objectColorLoc, 1.0f, 0.0f, 0.0f); // red
                    break;
            }
            
            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
    }
}

void GameRenderer::renderPlayer(
    const Player& player,
    Shader& shader,
    unsigned int cubeVAO
)
{
    const glm::vec2 position = player.getPosition();
    const glm::vec3& color = player.getColor();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, 0.1f, position.y));
    model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
    
    int modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
    glUniform3f(objectColorLoc, color.r, color.g, color.b);
    
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void GameRenderer::renderEnemy(
    const Enemy& enemy,
    Shader& shader,
    unsigned int cubeVAO
)
{
    const glm::vec2 position = enemy.get_position();
    const glm::vec3& color = enemy.getColor();
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, 0.1f, position.y));
    model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));

    int modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
    glUniform3f(objectColorLoc, color.r, color.g, color.b);

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void GameRenderer::renderTargetBeam(const Enemy& enemy, Shader& shader, unsigned int cubeVAO)
{
    glm::vec2 beam = enemy.getTarget() - enemy.get_position();
    float beam_length = glm::length(beam);

    if (beam_length < 0.001f)
    {
        return;
    }

    glm::vec2 beam_center = enemy.get_position() + beam * 0.5f;
    float beam_angle = -std::atan2(beam.y, beam.x);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(beam_center.x, 0.65f, beam_center.y));
    model = glm::rotate(model, beam_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(beam_length, 0.06f, 0.06f));

    int modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
    glUniform3f(objectColorLoc, enemy.getColor().r, enemy.getColor().g, enemy.getColor().b);

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void GameRenderer::render(const Game& game, Shader& shader, unsigned int cubeVAO)
{
    const GamePhase phase = game.getPhase();

    if (phase == GamePhase::MainMenu)
    {
        glDisable(GL_DEPTH_TEST);

        hud.renderMainMenu(
            static_cast<int>(
                game.getSelectedMenuOption()
            )
        );

        glEnable(GL_DEPTH_TEST);
        return;
    }

    if (phase == GamePhase::Scoreboard)
    {
        glDisable(GL_DEPTH_TEST);

        hud.renderScoreboard(
            game.getScoreboard()
        );

        glEnable(GL_DEPTH_TEST);
        return;
    }

    renderGrid(
        game.getGrid(),
        shader,
        cubeVAO
    );

    renderPlayer(
        game.getPlayer(),
        shader,
        cubeVAO
    );

    const auto enemies = game.getEnemies();

    for (const Enemy* enemy : enemies)
    {
        renderEnemy(
            *enemy,
            shader,
            cubeVAO
        );
    }

    glDisable(GL_DEPTH_TEST);

    const GameState& state = game.getGameState();

    hud.render(state);

    if (phase == GamePhase::Ready)
    {
        hud.renderReady();
    }
    else if (phase == GamePhase::Paused)
    {
        hud.renderPause(
            static_cast<int>(
                game.getSelectedPauseMenuOption()
            )
        );
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

        hud.renderHighScore(
            game.getEnteredName(),
            score,
            game.getScoreboard().getHighScore() < score
        );
    }

    glEnable(GL_DEPTH_TEST);
}