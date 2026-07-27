#pragma once
#include <glm/glm.hpp>
#include "Rect.hpp"
#include "GameState.hpp"

class Grid;
class Shader;

// Movement commands are relative to the camera direction.
enum class Direction
{
    Right,
    Left,
    Back,
    Forward
};

class Player
{
public:
    Player(float x, float y, GameState *state);

    // Grid-space movement helpers.
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

    bool canMoveTo(glm::vec2 targetPos, Grid& grid);
    bool update(Grid& grid, float deltaTime);
    bool collectPellet(int x, int y, Grid& grid);

    // Player state accessors.
    glm::vec2 getPosition() const { return visual_position; }
    glm::vec2 getCurrentDirection() const {return curr_direction; }
    glm::vec2 getCameraDirection() const {return camera_direction;}
    Rect getPlayerRect() const
    {
        return Rect{
        visual_position.x - HITBOX_SIZE / 2.0f,
        visual_position.y - HITBOX_SIZE / 2.0f,
        HITBOX_SIZE
        };
    }
    bool getCollided() const { return collided; }
    bool getEnergizer() const { return energizer; }
    void setPosition(float x, float y);
    void setPosition(const glm::vec2& pos);
    void setDirection(Direction direct, bool updateCamera = false);
    void setEnergizerTrue() { energizer = true; multiplier = 0; }

    void render(Shader& shader, unsigned int cubeVAO);
    void setCollided(bool collidedValue);
    void resetEnergizer() { energizer = false; }
    void killGhost();
    void resetPlayer();


    
private:
    GameState *gameState;
    glm::vec2 position;          // Logical grid position.
    glm::vec3 color;             // Render color.
    glm::vec2 visual_position;   // Interpolated draw position.
    glm::vec2 curr_direction;
    glm::vec2 target_direction;
    glm::vec2 camera_direction;
    glm::vec2 initialPosition;

    bool collided{false};
    bool energizer{false};
    int multiplier = 0;
    const float MOVEMENT_THRESHOLD_VIS = 0.1;
    const float MOVEMENT_THRESHOLD_POS = 0.05;
    static constexpr float SPEED = 3.0f;
    static constexpr float HITBOX_SIZE = 0.8f;
};
