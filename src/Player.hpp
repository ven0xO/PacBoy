#pragma once

#include <glm/glm.hpp>

#include "Rect.hpp"

class GameState;
class Grid;

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
    Player(float x, float y, GameState& state);

    bool update(Grid& grid, float deltaTime);
    bool collectPellet(int x, int y, Grid& grid);

    glm::vec2 getPosition() const
    {
        return visual_position;
    }
    glm::vec2 getCurrentDirection() const
    {
        return curr_direction;
    }
    glm::vec2 getCameraDirection() const
    {
        return camera_direction;
    }
    Rect getPlayerRect() const
    {
        return Rect{visual_position.x - HITBOX_SIZE / 2.0f, visual_position.y - HITBOX_SIZE / 2.0f,
                    HITBOX_SIZE};
    }
    const glm::vec3& getColor() const
    {
        return color;
    }
    bool getEnergizer() const
    {
        return energizer;
    }
    void setPosition(float x, float y);
    void setPosition(const glm::vec2& pos);
    void setDirection(Direction direct, bool updateCamera = false);
    void setEnergizerTrue()
    {
        energizer = true;
        multiplier = 0;
    }

    void resetEnergizer()
    {
        energizer = false;
    }
    void killGhost();
    void resetPlayer();
    void resetPlayer(const glm::vec2& spawnPosition);

private:
    GameState& gameState;
    glm::vec3 color;
    glm::vec2 visual_position;
    glm::vec2 curr_direction;
    glm::vec2 target_direction;
    glm::vec2 camera_direction;
    glm::vec2 initialPosition;

    bool energizer{false};
    int multiplier = 0;
    static constexpr float SPEED = 3.0f;
    static constexpr float HITBOX_SIZE = 0.8f;

    void move(float deltaTime);
};
