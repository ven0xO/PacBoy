#pragma once
#include <glm/glm.hpp>
#include "Grid.hpp"
#include "Rect.hpp"
#include "GameState.hpp"

class Player;
class Shader;

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

    Enemy(Type enemy_type, Grid* grid_in, Player* player_in, glm::vec2 start_pos, GameState* gmState); 

    glm::vec2 find_target();

    glm::vec2 get_position() const {return position;}
    glm::vec2 get_spawn_point() const {return spawn_point;}
    State get_state() const {return state;}

    void set_red_ghost(Enemy* red_ghost_v);
    void set_grid(Grid* grid_v);
    void set_state_dead()
    {
        state = State::Dead;
        target = spawn_entrance;
        state_change = false;
    }
    void set_position(glm::vec2 pos) { position = pos; }
    void assign_scatter();
    void update(float timer, int level);
    void calc_direction(glm::vec2 curr, glm::vec2 dest);
    void move();
    void render(Shader& shader, unsigned int cubeVAO);
    void renderTargetBeam(Shader& shader, unsigned int cubeVAO);
    bool checkCollision(const Rect& playerRect);
    void resetGhost();
    

private:
    Type type;
    State state = State::Scatter;
    State stateBeforeChange{State::Scatter};

    glm::vec2 target;
    glm::vec2 scatter_target;
    glm::vec2 position;
    glm::vec2 direction;
    glm::vec2 spawn_point;
    glm::vec2 spawn_entrance;
    glm::vec3 color;
    Rect enemyRect;

    bool is_at_center(glm::vec2 pos);
    bool is_spawn_gate(glm::vec2 pos);

    bool state_change = false;
    bool energizerChange = false;
    int last_timer {-1};
    float scaredUntil {0};

    Enemy* red_ghost{nullptr};
    Grid* grid;
    Player* player;

    const float SPEED = 0.05;
    static constexpr float HITBOX_SIZE = 0.8f;
    bool left_spawn = false;
};
