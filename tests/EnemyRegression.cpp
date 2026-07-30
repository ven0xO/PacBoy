#include "Enemy.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "Grid.hpp"
#include "Player.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

namespace
{
    constexpr float EPSILON{0.001f};
    const char* LEVEL_PATH{
        TEST_LEVEL_PATH
    };

    bool isNear(
        const glm::vec2& first,
        const glm::vec2& second
    )
    {
        return glm::length(first - second) < EPSILON;
    }

    void require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void testTargetingRules()
    {
        Grid grid;
        require(grid.loadFromFile(LEVEL_PATH), "level did not load");

        GameState state;
        const glm::vec2 playerSpawn =
            grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, &state);
        player.setPosition(20.0f, 20.0f);

        Enemy red(
            Type::Red,
            &grid,
            &player,
            grid.getRedGhostSpawnPosition()
        );
        Enemy pink(
            Type::Pink,
            &grid,
            &player,
            grid.getPinkGhostSpawnPosition()
        );
        Enemy blue(
            Type::Blue,
            &grid,
            &player,
            grid.getBlueGhostSpawnPosition()
        );
        Enemy orange(
            Type::Orange,
            &grid,
            &player,
            grid.getOrangeGhostSpawnPosition()
        );

        red.set_position(glm::vec2(5.0f, 5.0f));
        blue.set_red_ghost(&red);

        require(
            isNear(red.find_target(), glm::vec2(20.0f, 20.0f)),
            "red targeting changed"
        );
        require(
            isNear(pink.find_target(), glm::vec2(23.0f, 20.0f)),
            "pink targeting changed"
        );
        require(
            isNear(blue.find_target(), glm::vec2(39.0f, 35.0f)),
            "blue targeting changed"
        );

        orange.set_position(glm::vec2(20.0f, 20.0f));
        require(
            isNear(
                orange.find_target(),
                glm::vec2(
                    0.0f,
                    static_cast<float>(grid.getHeight())
                )
            ),
            "orange near-player targeting changed"
        );

        orange.set_position(glm::vec2(0.0f, 0.0f));
        require(
            isNear(
                orange.find_target(),
                glm::vec2(20.0f, 20.0f)
            ),
            "orange far-player targeting changed"
        );
    }

    void testReleaseDelays()
    {
        Grid grid;
        require(grid.loadFromFile(LEVEL_PATH), "level did not load");

        GameState state;
        const glm::vec2 playerSpawn =
            grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, &state);

        const std::array<Type, 4> types{
            Type::Red,
            Type::Pink,
            Type::Blue,
            Type::Orange
        };
        const std::array<glm::vec2, 4> spawns{
            grid.getRedGhostSpawnPosition(),
            grid.getPinkGhostSpawnPosition(),
            grid.getBlueGhostSpawnPosition(),
            grid.getOrangeGhostSpawnPosition()
        };
        const std::array<float, 4> delays{
            0.0f,
            2.0f,
            5.0f,
            8.0f
        };

        for (std::size_t index = 0; index < types.size(); ++index)
        {
            Enemy enemy(
                types[index],
                &grid,
                &player,
                spawns[index]
            );

            const glm::vec2 start = enemy.get_position();

            if (delays[index] > 0.0f)
            {
                enemy.update(
                    delays[index] - 0.01f,
                    1,
                    0.1f
                );
                require(
                    isNear(enemy.get_position(), start),
                    "ghost left before its release delay"
                );
            }

            enemy.update(delays[index], 1, 0.1f);
            require(
                !isNear(enemy.get_position(), start),
                "ghost did not leave at its release delay"
            );
        }
    }

    void testScaredState()
    {
        Grid grid;
        require(grid.loadFromFile(LEVEL_PATH), "level did not load");

        GameState gameState;
        const glm::vec2 playerSpawn =
            grid.getPacmanStartPosition();
        Player player(
            playerSpawn.x,
            playerSpawn.y,
            &gameState
        );
        Enemy enemy(
            Type::Red,
            &grid,
            &player,
            grid.getRedGhostSpawnPosition()
        );

        enemy.enterScared(0.0f);
        enemy.update(5.99f, 1, 5.99f);
        require(
            enemy.get_state() == State::Scared,
            "Scared ended too early"
        );

        enemy.update(6.0f, 1, 0.05f);
        require(
            enemy.get_state() == State::Scatter,
            "ghost did not restore its active mode"
        );

        enemy.update(6.01f, 1, 6.89f);
        require(
            enemy.get_state() == State::Scatter,
            "Scared incorrectly advanced the mode schedule"
        );

        enemy.update(6.02f, 1, 0.07f);
        require(
            enemy.get_state() == State::Chase,
            "mode schedule threshold changed"
        );
    }

    void testDeadGhostReturnsAndLeaves()
    {
        Grid grid;
        require(grid.loadFromFile(LEVEL_PATH), "level did not load");

        GameState gameState;
        const glm::vec2 playerSpawn =
            grid.getPacmanStartPosition();
        Player player(
            playerSpawn.x,
            playerSpawn.y,
            &gameState
        );
        Enemy enemy(
            Type::Red,
            &grid,
            &player,
            grid.getRedGhostSpawnPosition()
        );

        enemy.set_position(
            grid.getGhostExitPositions().front() +
            glm::vec2(0.0f, -1.0f)
        );
        enemy.set_state_dead();

        bool revived = false;
        float timer = 10.0f;

        for (int frame = 0; frame < 600; ++frame)
        {
            timer += 0.05f;
            enemy.update(timer, 1, 0.05f);

            if (enemy.get_state() != State::Dead)
            {
                revived = true;
                break;
            }
        }

        require(revived, "dead ghost did not revive");

        bool leftSpawn = false;

        for (int frame = 0; frame < 200; ++frame)
        {
            timer += 0.05f;
            enemy.update(timer, 1, 0.05f);

            if (
                glm::distance(
                    enemy.get_position(),
                    enemy.get_spawn_point()
                ) > 2.5f
            )
            {
                leftSpawn = true;
                break;
            }
        }

        require(leftSpawn, "revived ghost did not leave the spawn");
    }

    void testSharedGameOperations()
    {
        Game game({LEVEL_PATH});
        require(game.isInitialized(), "game did not initialize");
        require(game.startNewGame(0.0f), "new game did not start");

        auto initialEnemies = game.getEnemies();
        for (const Enemy* enemy : initialEnemies)
        {
            require(enemy != nullptr, "ghost collection is incomplete");
        }

        Player& player =
            const_cast<Player&>(game.getPlayer());
        player.setEnergizerTrue();
        game.update(2.0f, 0.0f);

        for (const Enemy* enemy : game.getEnemies())
        {
            require(
                enemy->get_state() == State::Scared,
                "energizer did not affect every ghost"
            );
        }

        game.resetRound(2.0f);
        const std::array<glm::vec2, 4> spawns{
            game.getGrid().getRedGhostSpawnPosition(),
            game.getGrid().getPinkGhostSpawnPosition(),
            game.getGrid().getBlueGhostSpawnPosition(),
            game.getGrid().getOrangeGhostSpawnPosition()
        };
        const auto resetEnemies = game.getEnemies();

        for (std::size_t index = 0; index < resetEnemies.size(); ++index)
        {
            require(
                resetEnemies[index]->get_state() == State::Scatter,
                "reset did not restore every ghost state"
            );
            require(
                isNear(
                    resetEnemies[index]->get_position(),
                    spawns[index]
                ),
                "reset did not restore every ghost position"
            );
        }
    }
}

int main()
{
    try
    {
        testTargetingRules();
        testReleaseDelays();
        testScaredState();
        testDeadGhostReturnsAndLeaves();
        testSharedGameOperations();
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "Enemy regression test failed: "
            << error.what() << '\n';
        return 1;
    }

    return 0;
}
