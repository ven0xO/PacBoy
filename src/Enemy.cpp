#include "Enemy.hpp"
#include "Player.hpp"

#include <random>
#include <algorithm>
#include <cmath>
#include <limits>
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

Enemy::Enemy(Type enemy_type, Grid* grid_in, Player* player_in, glm::vec2 start_pos, GameState* gmState) : 
type(enemy_type), 
color(color::get_enemy_color(enemy_type)),
grid(grid_in),
player(player_in),
target(start_pos),
position(start_pos),
direction(0.0f, 0.0f),
spawn_point(start_pos),
spawn_entrance(grid_in->getGhostEntryPosition()),
enemyRect{start_pos.x - HITBOX_SIZE / 2.0f, start_pos.y - HITBOX_SIZE / 2.0f, HITBOX_SIZE}
{
    assign_scatter();
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
    int currentSecond = static_cast<int>(timer);

    
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
        // Scatter/chase schedules change at fixed timestamps for each level range.
        if (currentSecond != last_timer)
        {
            last_timer = currentSecond;
            if(level == 1)
            {
                if(currentSecond == 7 || currentSecond == 27 || currentSecond == 34 || currentSecond == 54 || currentSecond == 59 || currentSecond == 79 || currentSecond == 84)
                {state_change = true;}


                if(currentSecond == 27 || currentSecond == 54 || currentSecond == 79)
                {
                    state = State::Scatter;
                } 
                else if(currentSecond == 7 || currentSecond == 34 || currentSecond == 59 || currentSecond >= 84)
                {
                    state = State::Chase;
                }
            }
            else if(level >= 2 && level <= 4)
            {
                if(currentSecond == 7 || currentSecond == 27 || currentSecond == 34 || currentSecond == 54 || currentSecond == 59)
                {state_change = true;}


                if(currentSecond == 27 || currentSecond == 54)
                {
                    state = State::Scatter;
                } 
                else if(currentSecond == 7 || currentSecond == 34 || currentSecond >= 59)
                {
                    state = State::Chase;
                }
            }
            else if(level >= 5)
            {
                if(currentSecond == 5 || currentSecond == 25 || currentSecond == 30 || currentSecond == 50 || currentSecond == 55)
                {state_change = true;}


                if(currentSecond == 25 || currentSecond == 50)
                {
                    state = State::Scatter;
                } 
                else if(currentSecond == 5 || currentSecond == 30 || currentSecond >= 55)
                {
                    state = State::Chase;
                }
            }
        }
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
            while (random_position == position - direction 
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
                target = grid->getGhostExitPosition();
                if(position == grid->getGhostExitPosition())
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
                if(pos != position - direction 
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
                target = grid->getGhostExitPosition();
                if(position == grid->getGhostExitPosition())
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
                if(pos != position - direction
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
        if(position == spawn_entrance) 
        {
            target = spawn_point;
        }

        if(position == spawn_point)
        {
            state = State::Chase;
            left_spawn = false;
            state_change = false;
            color = color::get_enemy_color(type);
            target = grid->getGhostExitPosition();
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
                if (possible_position == reversePosition && possible_positions.size() > 1)
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

    position = nextPosition;

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
    last_timer = -1;
    scaredUntil = 0.0f;
    color = color::get_enemy_color(type);
    direction = glm::vec2(0.0f, 0.0f);
}