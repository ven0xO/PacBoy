#include "Enemy.hpp"
#include "Player.hpp"

#include <random>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../external/shader_s.h"

namespace color
{
    const glm::vec3 red_color = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 cyan_color = glm::vec3(0.0f, 1.0f, 1.0f);
    const glm::vec3 pink_color = glm::vec3(1.0f, 0.4f, 0.7f);
    const glm::vec3 orange_color = glm::vec3(1.0f, 0.5f, 0.0f);
    const glm::vec3 blue_color = glm::vec3(0.0f, 0.2f, 1.0f);

    glm::vec3 get_enemy_color(Type type)
    {
        switch (type)
        {
        case Type::Red:
            return red_color;
        case Type::Pink:
            return pink_color;
        case Type::Blue:
            return cyan_color;
        case Type::Orange:
            return orange_color;
        }

        return red_color;
    }
}

namespace
{
    struct ModeTransition
    {
        float time;
        State nextState;
    };

    const std::vector<ModeTransition> LEVEL_ONE_SCHEDULE{
        {7.0f, State::Chase},
        {27.0f, State::Scatter},
        {34.0f, State::Chase},
        {54.0f, State::Scatter},
        {59.0f, State::Chase},
        {79.0f, State::Scatter},
        {84.0f, State::Chase}
    };

    const std::vector<ModeTransition> LEVELS_TWO_TO_FOUR_SCHEDULE{
        {7.0f, State::Chase},
        {27.0f, State::Scatter},
        {34.0f, State::Chase},
        {54.0f, State::Scatter},
        {59.0f, State::Chase}
    };

    const std::vector<ModeTransition> LEVEL_FIVE_PLUS_SCHEDULE{
        {5.0f, State::Chase},
        {25.0f, State::Scatter},
        {30.0f, State::Chase},
        {50.0f, State::Scatter},
        {55.0f, State::Chase}
    };

    const std::vector<ModeTransition>& getSchedule(int level)
    {
        if (level == 1)
        {
            return LEVEL_ONE_SCHEDULE;
        }

        if (level >= 2 && level <= 4)
        {
            return LEVELS_TWO_TO_FOUR_SCHEDULE;
        }

        return LEVEL_FIVE_PLUS_SCHEDULE;
    }

    glm::ivec2 toTileCoordinates(const glm::vec2& position)
    {
        return glm::ivec2{
            static_cast<int>(std::round(position.x)),
            static_cast<int>(std::round(position.y))
        };
    }

    bool isSameTile(
        const glm::vec2& first,
        const glm::vec2& second
    )
    {
        const glm::ivec2 firstTile = toTileCoordinates(first);
        const glm::ivec2 secondTile = toTileCoordinates(second);

        return firstTile.x == secondTile.x &&
            firstTile.y == secondTile.y;
    }

    glm::vec2 findClosestPosition(
        const std::vector<glm::vec2>& positions,
        const glm::vec2& reference
    )
    {
        return *std::min_element(
            positions.begin(),
            positions.end(),
            [&reference](const glm::vec2& first, const glm::vec2& second)
            {
                return glm::distance(first, reference) <
                       glm::distance(second, reference);
            }
        );
    }
}

Enemy::Enemy(Type enemy_type, Grid* grid_in, Player* player_in, glm::vec2 start_pos, GameState* gmState) : 
type(enemy_type), 
color(color::get_enemy_color(enemy_type)),
grid(grid_in),
player(player_in),
target(start_pos),
position(start_pos),
direction(0.0f, 0.0f),
spawn_point(start_pos),
enemyRect{start_pos.x - HITBOX_SIZE / 2.0f, start_pos.y - HITBOX_SIZE / 2.0f, HITBOX_SIZE}
{
    spawn_entrance = findClosestPosition(
        grid->getGhostEntryPositions(),
        spawn_point
    );
    spawn_exit = findClosestPosition(
        grid->getGhostExitPositions(),
        spawn_entrance
    );
    target = spawn_exit;

    assign_scatter();

    switch (type)
    {
        case Type::Red:
            releaseDelay = 0.0f;
            break;

        case Type::Pink:
            releaseDelay = 2.0f;
            break;

        case Type::Blue:
            releaseDelay = 5.0f;
            break;

        case Type::Orange:
            releaseDelay = 8.0f;
            break;
    }
}

void Enemy::set_red_ghost(Enemy* red_ghost_v)
{
    red_ghost = red_ghost_v;
}

void Enemy::set_grid(Grid* grid_v)
{
    grid = grid_v;
}

void Enemy::calc_direction(glm::vec2 curr, glm::vec2 dest)
{
    glm::vec2 delta = dest - curr;

    if (glm::length(delta) > 0.0f)
        direction = glm::normalize(delta);
    else
        direction = glm::vec2(0.0f, 0.0f);
}

void Enemy::assign_scatter()
{
    switch (type)
    {
    case Type::Red:
        
        scatter_target = glm::vec2(grid->getWidth(), 0.0f);
        break;
    case Type::Pink:
        
        scatter_target = glm::vec2(0.0f, 0.0f);
        break;
    case Type::Orange:
        
        scatter_target = glm::vec2(0.0f, grid->getHeight());
        break;
    case Type::Blue:
        
        scatter_target = glm::vec2(grid->getWidth(), grid->getHeight());
        break;
    
    default:
        break;
    }
}

glm::vec2 Enemy::find_target()
{
    glm::vec2 player_position = glm::round(player->getPosition());
    // Each ghost type follows a different Pac-Man-style targeting rule.
    if(type == Type::Red)
    {
        target = player_position;
        return target;
    }
    else if(type == Type::Pink)
    {
        target = player_position + 3.0f * player->getCurrentDirection();
        return target;
    }
    else if(type == Type::Orange)
    {
        if(glm::distance(player_position, position) <= 8)
        {
            target = scatter_target;
        }
        else target = player_position;
    }
    else if(type == Type::Blue)
    {
        glm::vec2 two_spaces = player_position + 2.0f * player->getCurrentDirection();
        target = 2.0f * two_spaces - red_ghost->get_position();
    }

    return target;
}

void Enemy::update(float timer, int level, float deltaTime)
{

    bool isScared = player->getEnergizer();

    if (state == State::Scared && timer >= scaredUntil)
    {
        color = color::get_enemy_color(type);
        state = stateBeforeChange;
    }
    if(isScared && state != State::Dead)
    {
        if(state != State::Scared)
        {
            stateBeforeChange = state;
        }
        state = State::Scared;
        scaredUntil = timer + 6;
        color = color::blue_color;
    }
    else if (state != State::Dead && timer >= scaredUntil)
    {
        const auto& schedule = getSchedule(level);

        while (scheduleIndex < schedule.size() &&
            timer >= schedule[scheduleIndex].time)
        {
            const ModeTransition& transition = schedule[scheduleIndex];

            if (state != transition.nextState)
            {
                state = transition.nextState;
                state_change = true;
            }

            ++scheduleIndex;
        }
    }

    if (!left_spawn && timer < releaseDelay)
    {
        direction = glm::vec2(0.0f, 0.0f);
        return;
    }
    
    if (!is_at_center(position))
    {
        move(deltaTime);
        return;
    }
    position = glm::round(position);

    // Direction changes are made only at tile centers to keep grid movement stable.
    if(state == State::Scared)
    {
        if(state_change)
        {
            direction *= -1.0f;
            state_change = false;
        } else
        {
            std::vector<glm::vec2> possible_positions = grid->possible_moves(position);

            static std::random_device rd;
            static std::mt19937 gen(rd());
            glm::vec2 random_position;

            do
            {
                std::uniform_int_distribution<std::size_t> dist(
                0,
                possible_positions.size() - 1
                );
                random_position = possible_positions[dist(gen)];
            }
            while (isSameTile(random_position, position - direction)
                    || grid->getTile(random_position.x, random_position.y) == Tile::GhostSpawnEntrance);

            calc_direction(position, random_position);
        }
    }
    else if(state == State::Chase)
    {
        if(state_change)
        {
            direction *= -1.0f;
            state_change = false;
        } else
        {
            if(!left_spawn)
            {
                target = spawn_exit;
                if(isSameTile(position, spawn_exit))
                {
                    left_spawn = true;
                }
            }
            else    find_target();
            
            std::vector<glm::vec2> possible_positions = grid->possible_moves(position);

            float best_val = std::numeric_limits<float>::max();
            glm::vec2 next_move = position;

            for(glm::vec2 pos : possible_positions)
            {
                if(!isSameTile(pos, position - direction)
                    && (!left_spawn || grid->getTile(pos.x, pos.y) != Tile::GhostSpawnEntrance)
                )
                {
                    float dist = glm::distance(pos, target);

                    if(dist < best_val)
                    {
                        best_val = dist;
                        next_move = pos;
                    }
                }
            }

            calc_direction(position, next_move);
        }
        
    }
    else if(state == State::Scatter)
    {
        if(state_change)
        {
            direction *= -1.0f;
            state_change = false;
        }else
        {
            if(!left_spawn)
            {
                target = spawn_exit;
                if(isSameTile(position, spawn_exit))
                {
                    left_spawn = true;
                }
            }
            else    target = scatter_target;
            
            std::vector<glm::vec2> possible_positions = grid->possible_moves(position);

            float best_val = std::numeric_limits<float>::max();
            glm::vec2 next_move = position;

            for(glm::vec2 pos : possible_positions)
            {
                if(!isSameTile(pos, position - direction)
                && (!left_spawn || grid->getTile(pos.x, pos.y) != Tile::GhostSpawnEntrance)
                )
                {
                    float dist = glm::distance(pos, target);

                    if(dist < best_val)
                    {
                        best_val = dist;
                        next_move = pos;
                    }
                }
            }

            calc_direction(position, next_move);
        }
    }
    else if(state == State::Dead)
    {
        if (isSameTile(position, spawn_entrance))
        {
            target = spawn_point;
        }

        if (isSameTile(position, spawn_point))
        {
            state = State::Chase;
            left_spawn = false;
            state_change = false;
            color = color::get_enemy_color(type);
            target = spawn_exit;
            calc_direction(position, target);
        }
        else
        {
            std::vector<glm::vec2> possible_positions = grid->possible_moves(position);

            float bestDistance = std::numeric_limits<float>::max();
            glm::vec2 next_move = position;

            glm::vec2 reversePosition = position - direction;


            for (const glm::vec2& possible_position : possible_positions)
            {
                if (isSameTile(possible_position, reversePosition) && possible_positions.size() > 1)
                {
                    continue;
                }

                float distance = glm::distance(possible_position, target);

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    next_move = possible_position;
                }
            }
            calc_direction(position, next_move);
        }
        
    }

    move(deltaTime);
}

void Enemy::move(float deltaTime)
{
    constexpr float EPSILON = 0.0001f;

    float movement = SPEED * deltaTime;
    glm::vec2 nextPosition = position + direction * movement;

    if (direction.x > 0.0f)
    {
        float nextCenter = std::floor(position.x + EPSILON) + 1.0f;

        if (nextPosition.x >= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(position.y);
        }
    }
    else if (direction.x < 0.0f)
    {
        float nextCenter = std::ceil(position.x - EPSILON) - 1.0f;

        if (nextPosition.x <= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(position.y);
        }
    }
    else if (direction.y > 0.0f)
    {
        float nextCenter = std::floor(position.y + EPSILON) + 1.0f;

        if (nextPosition.y >= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(position.x);
        }
    }
    else if (direction.y < 0.0f)
    {
        float nextCenter = std::ceil(position.y - EPSILON) - 1.0f;

        if (nextPosition.y <= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(position.x);
        }
    }

    position = grid->wrapPosition(nextPosition);

    enemyRect.x = position.x - HITBOX_SIZE / 2.0f;
    enemyRect.y = position.y - HITBOX_SIZE / 2.0f;
}

void Enemy::render(Shader& shader, unsigned int cubeVAO)
{
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

void Enemy::renderTargetBeam(Shader& shader, unsigned int cubeVAO)
{
    glm::vec2 beam = target - position;
    float beam_length = glm::length(beam);

    if (beam_length < 0.001f)
    {
        return;
    }

    glm::vec2 beam_center = position + beam * 0.5f;
    float beam_angle = -std::atan2(beam.y, beam.x);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(beam_center.x, 0.65f, beam_center.y));
    model = glm::rotate(model, beam_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(beam_length, 0.06f, 0.06f));

    int modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
    glUniform3f(objectColorLoc, color.r, color.g, color.b);

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

bool Enemy::is_at_center(glm::vec2 pos)
{
    return glm::length(pos - glm::round(pos)) < 0.001f;
}

bool Enemy::is_spawn_gate(glm::vec2 pos)
{
    return grid->getTile(pos.x, pos.y) == Tile::GhostSpawnEntrance;
}

bool Enemy::checkCollision(const Rect& playerRect)
{
    return enemyRect.x < playerRect.x + playerRect.w &&
           enemyRect.x + enemyRect.w > playerRect.x &&
           enemyRect.y < playerRect.y + playerRect.w &&
           enemyRect.y + enemyRect.w > playerRect.y;
}

void Enemy::resetGhost()
{
    set_position(get_spawn_point());
    state_change = false;
    energizerChange = false;
    left_spawn = false;
    state = State::Scatter;
    stateBeforeChange = State::Scatter;
    scaredUntil = 0.0f;
    color = color::get_enemy_color(type);
    direction = glm::vec2(0.0f, 0.0f);
    target = spawn_exit;
    enemyRect.x = position.x - HITBOX_SIZE / 2.0f;
    enemyRect.y = position.y - HITBOX_SIZE / 2.0f;
    scheduleIndex = 0;
}
