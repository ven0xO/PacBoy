#include "Enemy.hpp"
#include "Grid.hpp"
#include "Player.hpp"

#include <random>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
    struct EnemyConfig
    {
        glm::vec3 normalColor;
        float releaseDelay;
        glm::vec2 scatterCorner;
    };

    const std::array<EnemyConfig, 4> ENEMY_CONFIGS{{
        {
            glm::vec3(1.0f, 0.0f, 0.0f),
            0.0f,
            glm::vec2(1.0f, 0.0f)
        },
        {
            glm::vec3(1.0f, 0.4f, 0.7f),
            2.0f,
            glm::vec2(0.0f, 0.0f)
        },
        {
            glm::vec3(0.0f, 1.0f, 1.0f),
            5.0f,
            glm::vec2(1.0f, 1.0f)
        },
        {
            glm::vec3(1.0f, 0.5f, 0.0f),
            8.0f,
            glm::vec2(0.0f, 1.0f)
        }
    }};

    const glm::vec3 SCARED_COLOR{
        0.0f,
        0.2f,
        1.0f
    };

    constexpr float SCARED_DURATION{6.0f};
    constexpr float PINK_LOOKAHEAD_TILES{3.0f};
    constexpr float BLUE_LOOKAHEAD_TILES{2.0f};
    constexpr float ORANGE_SCATTER_DISTANCE{8.0f};
    constexpr float CENTER_TOLERANCE{0.001f};
    constexpr float MOVEMENT_EPSILON{0.0001f};

    const EnemyConfig& getEnemyConfig(Type type)
    {
        return ENEMY_CONFIGS[static_cast<std::size_t>(type)];
    }

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

    std::vector<glm::vec2> getCandidateMoves(
        Grid& grid,
        const glm::vec2& position,
        const glm::vec2& direction,
        bool allowSpawnGate
    )
    {
        std::vector<glm::vec2> moves =
            grid.possible_moves(position);

        if (!allowSpawnGate)
        {
            moves.erase(
                std::remove_if(
                    moves.begin(),
                    moves.end(),
                    [&grid](const glm::vec2& move)
                    {
                        return grid.getTile(move.x, move.y) ==
                            Tile::GhostSpawnEntrance;
                    }
                ),
                moves.end()
            );
        }

        if (moves.size() > 1)
        {
            const glm::vec2 reversePosition =
                position - direction;

            moves.erase(
                std::remove_if(
                    moves.begin(),
                    moves.end(),
                    [&reversePosition](const glm::vec2& move)
                    {
                        return isSameTile(move, reversePosition);
                    }
                ),
                moves.end()
            );
        }

        return moves;
    }

    glm::vec2 getClosestMove(
        const std::vector<glm::vec2>& moves,
        const glm::vec2& position,
        const glm::vec2& target
    )
    {
        if (moves.empty())
        {
            return position;
        }

        return *std::min_element(
            moves.begin(),
            moves.end(),
            [&target](const glm::vec2& first, const glm::vec2& second)
            {
                return glm::distance(first, target) <
                    glm::distance(second, target);
            }
        );
    }

    glm::vec2 getRandomMove(
        const std::vector<glm::vec2>& moves,
        const glm::vec2& position
    )
    {
        if (moves.empty())
        {
            return position;
        }

        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());

        std::uniform_int_distribution<std::size_t> distribution(
            0,
            moves.size() - 1
        );

        return moves[distribution(generator)];
    }
}

Enemy::Enemy(
    Type enemy_type,
    Grid& grid_in,
    Player& player_in,
    glm::vec2 start_pos
)
    : type(enemy_type),
      target(start_pos),
      position(start_pos),
      direction(0.0f, 0.0f),
      spawn_point(start_pos),
      color(getEnemyConfig(enemy_type).normalColor),
      enemyRect{
          start_pos.x - HITBOX_SIZE / 2.0f,
          start_pos.y - HITBOX_SIZE / 2.0f,
          HITBOX_SIZE
      },
      grid(grid_in),
      player(player_in)
{
    spawn_entrance = findClosestPosition(
        grid.getGhostEntryPositions(),
        spawn_point
    );
    spawn_exit = findClosestPosition(
        grid.getGhostExitPositions(),
        spawn_entrance
    );
    target = spawn_exit;

    assign_scatter();
    releaseDelay = getEnemyConfig(type).releaseDelay;
}

void Enemy::set_red_ghost(const Enemy& red_ghost_v)
{
    red_ghost = &red_ghost_v;
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
    const glm::vec2 corner =
        getEnemyConfig(type).scatterCorner;

    scatter_target = glm::vec2(
        corner.x * static_cast<float>(grid.getWidth()),
        corner.y * static_cast<float>(grid.getHeight())
    );
}

glm::vec2 Enemy::find_target()
{
    glm::vec2 player_position = glm::round(player.getPosition());
    // Each ghost type follows a different Pac-Man-style targeting rule.
    if(type == Type::Red)
    {
        target = player_position;
        return target;
    }
    else if(type == Type::Pink)
    {
        target =
            player_position +
            PINK_LOOKAHEAD_TILES *
                player.getCurrentDirection();
        return target;
    }
    else if(type == Type::Orange)
    {
        if (
            glm::distance(player_position, position) <=
            ORANGE_SCATTER_DISTANCE
        )
        {
            target = scatter_target;
        }
        else target = player_position;
    }
    else if(type == Type::Blue)
    {
        glm::vec2 two_spaces =
            player_position +
            BLUE_LOOKAHEAD_TILES *
                player.getCurrentDirection();
        target = 2.0f * two_spaces - red_ghost->get_position();
    }

    return target;
}

void Enemy::enterScared(float timer)
{
    scaredUntil = timer + SCARED_DURATION;

    if (state == State::Dead)
    {
        return;
    }

    state = State::Scared;
    state_change = true;
    color = SCARED_COLOR;
}

void Enemy::updateState(
    float timer,
    int level,
    float deltaTime
)
{
    const bool scaredPeriodActive = timer < scaredUntil;

    if (state == State::Scared && !scaredPeriodActive)
    {
        color = getEnemyConfig(type).normalColor;
        state = activeMode;
    }

    // The Scatter/Chase schedule pauses for the whole Scared period.
    if (!scaredPeriodActive)
    {
        scheduleTimer += deltaTime;
        const auto& schedule = getSchedule(level);

        while (scheduleIndex < schedule.size() &&
            scheduleTimer >= schedule[scheduleIndex].time)
        {
            const ModeTransition& transition = schedule[scheduleIndex];
            activeMode = transition.nextState;

            if (
                (state == State::Chase || state == State::Scatter) &&
                state != activeMode
            )
            {
                state = activeMode;
                state_change = true;
            }

            ++scheduleIndex;
        }
    }
}

bool Enemy::reverseDirectionIfNeeded(bool allowSpawnGate)
{
    if (!state_change)
    {
        return false;
    }

    state_change = false;

    if (glm::length(direction) == 0.0f)
    {
        return false;
    }

    const glm::vec2 reversePosition =
        position - direction;

    const Tile reverseTile =
        grid.getTile(reversePosition.x, reversePosition.y);

    if (
        reverseTile == Tile::Wall ||
        (
            !allowSpawnGate &&
            reverseTile == Tile::GhostSpawnEntrance
        )
    )
    {
        return false;
    }

    direction *= -1.0f;
    return true;
}

bool Enemy::chooseNextDirection(
    bool allowSpawnGate,
    bool directionReversed
)
{
    // Direction changes are made only at tile centers to keep grid movement stable.
    if(state == State::Scared)
    {
        if (!directionReversed)
        {
            const std::vector<glm::vec2> moves =
                getCandidateMoves(
                    grid,
                    position,
                    direction,
                    allowSpawnGate
                );

            if (!left_spawn)
            {
                target = spawn_exit;
                calc_direction(
                    position,
                    getClosestMove(moves, position, target)
                );
            }
            else
            {
                calc_direction(
                    position,
                    getRandomMove(moves, position)
                );
            }
        }
    }
    else if(state == State::Chase || state == State::Scatter)
    {
        if (!directionReversed)
        {
            if(!left_spawn)
            {
                target = spawn_exit;
            }
            else if (state == State::Chase)
            {
                find_target();
            }
            else
            {
                target = scatter_target;
            }

            const std::vector<glm::vec2> moves =
                getCandidateMoves(
                    grid,
                    position,
                    direction,
                    allowSpawnGate
                );

            calc_direction(
                position,
                getClosestMove(moves, position, target)
            );
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
            state = activeMode;
            left_spawn = false;
            state_change = false;
            color = getEnemyConfig(type).normalColor;
            target = spawn_exit;
            direction = glm::vec2(0.0f, 0.0f);
            return false;
        }

        if (!directionReversed)
        {
            const std::vector<glm::vec2> moves =
                getCandidateMoves(
                    grid,
                    position,
                    direction,
                    true
                );

            calc_direction(
                position,
                getClosestMove(moves, position, target)
            );
        }
    }

    return true;
}

void Enemy::update(float timer, int level, float deltaTime)
{
    updateState(timer, level, deltaTime);

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

    if (!left_spawn && isSameTile(position, spawn_exit))
    {
        left_spawn = true;
    }

    const bool allowSpawnGate =
        !left_spawn || state == State::Dead;

    const bool directionReversed =
        reverseDirectionIfNeeded(allowSpawnGate);

    if (chooseNextDirection(allowSpawnGate, directionReversed))
    {
        move(deltaTime);
    }
}

void Enemy::move(float deltaTime)
{
    float movement = SPEED * deltaTime;
    glm::vec2 nextPosition = position + direction * movement;

    if (direction.x > 0.0f)
    {
        float nextCenter =
            std::floor(position.x + MOVEMENT_EPSILON) + 1.0f;

        if (nextPosition.x >= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(position.y);
        }
    }
    else if (direction.x < 0.0f)
    {
        float nextCenter =
            std::ceil(position.x - MOVEMENT_EPSILON) - 1.0f;

        if (nextPosition.x <= nextCenter)
        {
            nextPosition.x = nextCenter;
            nextPosition.y = std::round(position.y);
        }
    }
    else if (direction.y > 0.0f)
    {
        float nextCenter =
            std::floor(position.y + MOVEMENT_EPSILON) + 1.0f;

        if (nextPosition.y >= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(position.x);
        }
    }
    else if (direction.y < 0.0f)
    {
        float nextCenter =
            std::ceil(position.y - MOVEMENT_EPSILON) - 1.0f;

        if (nextPosition.y <= nextCenter)
        {
            nextPosition.y = nextCenter;
            nextPosition.x = std::round(position.x);
        }
    }

    position = grid.wrapPosition(nextPosition);

    enemyRect.x = position.x - HITBOX_SIZE / 2.0f;
    enemyRect.y = position.y - HITBOX_SIZE / 2.0f;
}

bool Enemy::is_at_center(glm::vec2 pos)
{
    return
        glm::length(pos - glm::round(pos)) <
        CENTER_TOLERANCE;
}

bool Enemy::checkCollision(const Rect& playerRect) const
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
    left_spawn = false;
    state = State::Scatter;
    activeMode = State::Scatter;
    scaredUntil = 0.0f;
    scheduleTimer = 0.0f;
    color = getEnemyConfig(type).normalColor;
    direction = glm::vec2(0.0f, 0.0f);
    target = spawn_exit;
    enemyRect.x = position.x - HITBOX_SIZE / 2.0f;
    enemyRect.y = position.y - HITBOX_SIZE / 2.0f;
    scheduleIndex = 0;
}

void Enemy::resetGhost(const glm::vec2& spawnPosition)
{
    spawn_point = spawnPosition;
    spawn_entrance = findClosestPosition(
        grid.getGhostEntryPositions(),
        spawn_point
    );
    spawn_exit = findClosestPosition(
        grid.getGhostExitPositions(),
        spawn_entrance
    );
    assign_scatter();
    resetGhost();
}
