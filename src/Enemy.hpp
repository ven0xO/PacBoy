#pragma once

#include <glm/glm.hpp>

#include <cstddef>

#include "Rect.hpp"

class Grid;
class Player;

enum class State
{
    Chase,
    Scatter,
    Dead,
    Scared
};

enum class Type
{
    Red,
    Pink,
    Blue,
    Orange
};

class Enemy
{
public:
    Enemy(Type enemy_type, Grid& grid_in, Player& player_in, glm::vec2 start_pos);

    glm::vec2 find_target();

    glm::vec2 get_position() const
    {
        return position;
    }
    glm::vec2 get_spawn_point() const
    {
        return spawn_point;
    }
    State get_state() const
    {
        return state;
    }
    const glm::vec3& getColor() const
    {
        return color;
    }

    void set_red_ghost(const Enemy& red_ghost_v);
    void set_state_dead()
    {
        state = State::Dead;
        target = spawn_entrance;
        state_change = false;
    }
    void set_position(glm::vec2 pos)
    {
        position = pos;
    }
    void assign_scatter();
    void enterScared(float timer);
    void update(float timer, int level, float deltaTime);
    void move(float deltaTime);
    void calc_direction(glm::vec2 curr, glm::vec2 dest);
    bool checkCollision(const Rect& playerRect) const;
    void resetGhost();
    void resetGhost(const glm::vec2& spawnPosition);

private:
    Type type;
    State state = State::Scatter;
    State activeMode{State::Scatter};

    glm::vec2 target;
    glm::vec2 scatter_target;
    glm::vec2 position;
    glm::vec2 direction;
    glm::vec2 spawn_point;
    glm::vec2 spawn_entrance;
    glm::vec2 spawn_exit;
    glm::vec3 color;
    Rect enemyRect;

    bool is_at_center(glm::vec2 pos);
    void updateState(float timer, int level, float deltaTime);
    bool reverseDirectionIfNeeded(bool allowSpawnGate);
    bool chooseNextDirection(bool allowSpawnGate, bool directionReversed);

    bool state_change = false;
    float scaredUntil{0};
    float releaseDelay{0.0f};
    float scheduleTimer{0.0f};
    std::size_t scheduleIndex{0};

    // Only Blue uses this optional, non-owning link.
    const Enemy* red_ghost{nullptr};
    Grid& grid;
    Player& player;

    static constexpr float SPEED = 3.0f;
    static constexpr float HITBOX_SIZE = 0.8f;
    bool left_spawn = false;
};
