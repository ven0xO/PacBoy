#include "Player.hpp"
#include "GameState.hpp"
#include "Grid.hpp"

#include <glm/glm.hpp>

#include <cmath>

namespace
{
    glm::ivec2 toTileCoordinates(const glm::vec2& position)
    {
        return glm::ivec2{static_cast<int>(std::round(position.x)),
                          static_cast<int>(std::round(position.y))};
    }
} // namespace

Player::Player(float x, float y, GameState& state)
    : gameState(state), color(1.0f, 1.0f, 0.0f), visual_position(x, y), curr_direction(1.0f, 0.0f),
      target_direction(1.0f, 0.0f), camera_direction(1.0f, 0.0f), initialPosition(x, y)
{
}

CollectedTile Player::collectPellet(int x, int y, Grid& grid)
{
    Tile tile = grid.getTile(x, y);
    if (tile == Tile::Pellet)
    {
        gameState.addScore(10);
        grid.collectTile(x, y);
        gameState.collectPellet();
        return CollectedTile::Pellet;
    }
    else if (tile == Tile::Energizer)
    {
        gameState.addScore(50);
        grid.collectTile(x, y);
        gameState.collectEnergizer();
        setEnergizerTrue();
        return CollectedTile::Energizer;
    }
    return CollectedTile::None;
}

void Player::setDirection(Direction direct, bool updateCamera)
{
    // Convert camera-relative input into one of the four grid directions.
    glm::vec2 snapped_camera_dir = camera_direction;
    if (glm::abs(snapped_camera_dir.x) > glm::abs(snapped_camera_dir.y))
    {
        snapped_camera_dir = glm::vec2(glm::sign(snapped_camera_dir.x), 0.0f);
    }
    else
    {
        snapped_camera_dir = glm::vec2(0.0f, glm::sign(snapped_camera_dir.y));
    }

    switch (direct)
    {
        case Direction::Forward:
            target_direction = snapped_camera_dir;
            break;
        case Direction::Back:
            target_direction = -snapped_camera_dir;
            break;
        case Direction::Left:
            target_direction = glm::vec2(snapped_camera_dir.y, -snapped_camera_dir.x);
            break;
        case Direction::Right:
            target_direction = glm::vec2(-snapped_camera_dir.y, snapped_camera_dir.x);
            break;
    }
    if (updateCamera)
    {
        curr_direction = target_direction;
        camera_direction = target_direction;
    }
}

CollectedTile Player::update(Grid& grid, float deltaTime)
{
    CollectedTile collectedTile = CollectedTile::None;

    if (curr_direction != glm::vec2(0.0f, 0.0f) &&
        glm::length(curr_direction - camera_direction) > 0.01f)
    {
        constexpr float CAMERA_TURN_SPEED = 5.0f;
        float turnFactor = 1.0f - std::exp(-CAMERA_TURN_SPEED * deltaTime);

        camera_direction = glm::mix(camera_direction, curr_direction, turnFactor);
    }
    constexpr float CENTER_EPSILON = 0.001f;
    const glm::vec2 tileCenter = glm::round(visual_position);

    if (glm::length(visual_position - tileCenter) < CENTER_EPSILON)
    {
        visual_position = tileCenter;
        // Turn and collision decisions are only stable near tile centers.
        const glm::ivec2 currentTile = toTileCoordinates(visual_position);
        collectedTile = collectPellet(currentTile.x, currentTile.y, grid);

        glm::ivec2 nextTile = toTileCoordinates(visual_position + curr_direction);
        Tile new_pos_tile = grid.getTile(nextTile.x, nextTile.y);

        if (new_pos_tile == Tile::Wall)
        {
            nextTile = toTileCoordinates(visual_position + target_direction);
            new_pos_tile = grid.getTile(nextTile.x, nextTile.y);

            if (new_pos_tile != Tile::Wall)
            {
                curr_direction = target_direction;
            }
            else
            {
                curr_direction = glm::vec2(0.0f, 0.0f);
            }
        }
        else
        {
            nextTile = toTileCoordinates(visual_position + target_direction);
            new_pos_tile = grid.getTile(nextTile.x, nextTile.y);
            if (new_pos_tile != Tile::Wall)
            {
                curr_direction = target_direction;
            }
        }
    }

    move(deltaTime);
    visual_position = grid.wrapPosition(visual_position);

    return collectedTile;
}

void Player::setPosition(float x, float y)
{
    visual_position.x = x;
    visual_position.y = y;
}

void Player::setPosition(const glm::vec2& pos)
{
    visual_position = pos;
}

void Player::killGhost()
{
    static constexpr int ghostScores[] = {200, 400, 800, 1600};

    gameState.addScore(ghostScores[multiplier]);

    if (multiplier < 3)
    {
        multiplier++;
    }
}

void Player::resetPlayer()
{
    setPosition(initialPosition);

    curr_direction = glm::vec2(0.0f, 0.0f);
    target_direction = glm::vec2(0.0f, 0.0f);
    camera_direction = glm::vec2(1.0f, 0.0f);
    energizer = false;
    multiplier = 0;
}

void Player::resetPlayer(const glm::vec2& spawnPosition)
{
    initialPosition = spawnPosition;
    resetPlayer();
}

void Player::move(float deltaTime)
{
    constexpr float EPSILON = 0.0001f;

    const float movement = SPEED * deltaTime;
    glm::vec2 nextPosition = visual_position + curr_direction * movement;

    if (curr_direction.x > 0.0f)
    {
        const float nextCenter = std::floor(visual_position.x + EPSILON) + 1.0f;

        if (nextPosition.x >= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(visual_position.y);
        }
    }
    else if (curr_direction.x < 0.0f)
    {
        const float nextCenter = std::ceil(visual_position.x - EPSILON) - 1.0f;

        if (nextPosition.x <= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(visual_position.y);
        }
    }
    else if (curr_direction.y > 0.0f)
    {
        const float nextCenter = std::floor(visual_position.y + EPSILON) + 1.0f;

        if (nextPosition.y >= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(visual_position.x);
        }
    }
    else if (curr_direction.y < 0.0f)
    {
        const float nextCenter = std::ceil(visual_position.y - EPSILON) - 1.0f;

        if (nextPosition.y <= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(visual_position.x);
        }
    }

    visual_position = nextPosition;
}
