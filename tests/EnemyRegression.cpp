#include "Enemy.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "Grid.hpp"
#include "Player.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

static_assert(std::is_constructible_v<Player, float, float, GameState&>);
static_assert(!std::is_constructible_v<Player, float, float, GameState*>);
static_assert(std::is_constructible_v<Enemy, Type, Grid&, Player&, glm::vec2>);
static_assert(!std::is_constructible_v<Enemy, Type, Grid*, Player*, glm::vec2>);

namespace
{
    constexpr float EPSILON{0.001f};
    const char* LEVEL_PATH{TEST_LEVEL_PATH};

    constexpr const char* DECISION_LEVEL{"#########\n"
                                         "#P     .#\n"
                                         "#      ##\n"
                                         "#       #\n"
                                         "#r pbo  #\n"
                                         "#   SE  #\n"
                                         "#########\n"};

    class TemporaryEnemyLevel
    {
    public:
        TemporaryEnemyLevel()
        {
            static unsigned long nextId{0};
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

            path = std::filesystem::temp_directory_path() /
                   ("pacboy-enemy-tests-" + std::to_string(timestamp) + "-" +
                    std::to_string(nextId++) + ".txt");

            std::ofstream output(path);
            output << DECISION_LEVEL;

            if (!output)
            {
                throw std::runtime_error("Cannot create temporary enemy level");
            }
        }

        ~TemporaryEnemyLevel()
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }

        TemporaryEnemyLevel(const TemporaryEnemyLevel&) = delete;
        TemporaryEnemyLevel& operator=(const TemporaryEnemyLevel&) = delete;

        std::string getPath() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    bool isNear(const glm::vec2& first, const glm::vec2& second)
    {
        return glm::length(first - second) < EPSILON;
    }

    void markAsReleased(Enemy& enemy, const Grid& grid)
    {
        enemy.set_position(grid.getGhostExitPositions().front());
        enemy.update(0.0f, 1, 0.0f);
    }

    // Catch2 intentionally decomposes assertion expressions through overloaded operators.
    // NOLINTBEGIN(bugprone-chained-comparison)

    TEST_CASE("Ghosts follow their type-specific targeting rules", "[enemy][targeting]")
    {
        Grid grid;
        REQUIRE(grid.loadFromFile(LEVEL_PATH));

        GameState state;
        const glm::vec2 playerSpawn = grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, state);
        player.setPosition(20.0f, 20.0f);

        Enemy red(Type::Red, grid, player, grid.getRedGhostSpawnPosition());
        Enemy pink(Type::Pink, grid, player, grid.getPinkGhostSpawnPosition());
        Enemy blue(Type::Blue, grid, player, grid.getBlueGhostSpawnPosition());
        Enemy orange(Type::Orange, grid, player, grid.getOrangeGhostSpawnPosition());

        red.set_position(glm::vec2(5.0f, 5.0f));
        blue.set_red_ghost(red);

        REQUIRE(isNear(red.find_target(), glm::vec2(20.0f, 20.0f)));
        REQUIRE(isNear(pink.find_target(), glm::vec2(23.0f, 20.0f)));
        REQUIRE(isNear(blue.find_target(), glm::vec2(39.0f, 35.0f)));

        orange.set_position(glm::vec2(20.0f, 20.0f));
        REQUIRE(
            isNear(orange.find_target(), glm::vec2(0.0f, static_cast<float>(grid.getHeight()))));

        orange.set_position(glm::vec2(0.0f, 0.0f));
        REQUIRE(isNear(orange.find_target(), glm::vec2(20.0f, 20.0f)));
    }

    TEST_CASE("Ghosts respect their release delays", "[enemy][release]")
    {
        Grid grid;
        REQUIRE(grid.loadFromFile(LEVEL_PATH));

        GameState state;
        const glm::vec2 playerSpawn = grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, state);

        const std::array<Type, 4> types{Type::Red, Type::Pink, Type::Blue, Type::Orange};
        const std::array<glm::vec2, 4> spawns{
            grid.getRedGhostSpawnPosition(), grid.getPinkGhostSpawnPosition(),
            grid.getBlueGhostSpawnPosition(), grid.getOrangeGhostSpawnPosition()};
        const std::array<float, 4> delays{0.0f, 2.0f, 5.0f, 8.0f};

        for (std::size_t index = 0; index < types.size(); ++index)
        {
            Enemy enemy(types[index], grid, player, spawns[index]);

            const glm::vec2 start = enemy.get_position();

            if (delays[index] > 0.0f)
            {
                enemy.update(delays[index] - 0.01f, 1, 0.1f);
                REQUIRE(isNear(enemy.get_position(), start));
            }

            enemy.update(delays[index], 1, 0.1f);
            REQUIRE_FALSE(isNear(enemy.get_position(), start));
        }
    }

    TEST_CASE("Scared pauses the mode schedule and restores the active mode",
              "[enemy][scared][schedule]")
    {
        Grid grid;
        REQUIRE(grid.loadFromFile(LEVEL_PATH));

        GameState gameState;
        const glm::vec2 playerSpawn = grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        enemy.enterScared(0.0f);
        enemy.update(5.99f, 1, 5.99f);
        REQUIRE(enemy.get_state() == State::Scared);

        enemy.update(6.0f, 1, 0.05f);
        REQUIRE(enemy.get_state() == State::Scatter);

        enemy.update(6.01f, 1, 6.89f);
        REQUIRE(enemy.get_state() == State::Scatter);

        enemy.update(6.02f, 1, 0.07f);
        REQUIRE(enemy.get_state() == State::Chase);
    }

    TEST_CASE("Scared entered during Chase restores Chase before the next Scatter",
              "[enemy][scared][schedule]")
    {
        Grid grid;
        REQUIRE(grid.loadFromFile(LEVEL_PATH));

        GameState gameState;
        const glm::vec2 playerSpawn = grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        enemy.update(7.0f, 1, 7.0f);
        REQUIRE(enemy.get_state() == State::Chase);

        enemy.enterScared(7.0f);
        enemy.update(12.99f, 1, 5.99f);
        REQUIRE(enemy.get_state() == State::Scared);

        enemy.update(13.0f, 1, 0.01f);
        REQUIRE(enemy.get_state() == State::Chase);

        enemy.update(32.0f, 1, 19.0f);
        REQUIRE(enemy.get_state() == State::Chase);

        enemy.update(33.0f, 1, 1.0f);
        REQUIRE(enemy.get_state() == State::Scatter);
    }

    TEST_CASE("Dead ghosts return to spawn, revive, and leave again", "[enemy][dead][spawn]")
    {
        Grid grid;
        REQUIRE(grid.loadFromFile(LEVEL_PATH));

        GameState gameState;
        const glm::vec2 playerSpawn = grid.getPacmanStartPosition();
        Player player(playerSpawn.x, playerSpawn.y, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        enemy.set_position(grid.getGhostExitPositions().front() + glm::vec2(0.0f, -1.0f));
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

        REQUIRE(revived);

        bool leftSpawn = false;

        for (int frame = 0; frame < 200; ++frame)
        {
            timer += 0.05f;
            enemy.update(timer, 1, 0.05f);

            if (glm::distance(enemy.get_position(), enemy.get_spawn_point()) > 2.5f)
            {
                leftSpawn = true;
                break;
            }
        }

        REQUIRE(leftSpawn);
    }

    TEST_CASE("Ghost chooses the closest legal path at an intersection",
              "[enemy][intersection][decision]")
    {
        TemporaryEnemyLevel level;
        Grid grid;
        REQUIRE(grid.loadFromFile(level.getPath()));

        GameState gameState;
        Player player(1.0f, 1.0f, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        markAsReleased(enemy, grid);

        enemy.set_position({4.0f, 2.0f});
        enemy.calc_direction({4.0f, 2.0f}, {5.0f, 2.0f});
        enemy.update(1.0f, 1, 1.0f / 3.0f);

        CHECK(isNear(enemy.get_position(), {5.0f, 2.0f}));
    }

    TEST_CASE("Ghost crosses a tile center consistently across deltaTime values",
              "[enemy][movement][delta-time]")
    {
        TemporaryEnemyLevel level;
        Grid grid;
        REQUIRE(grid.loadFromFile(level.getPath()));

        GameState coarseState;
        Player coarsePlayer(1.0f, 1.0f, coarseState);
        Enemy coarseEnemy(Type::Red, grid, coarsePlayer, grid.getRedGhostSpawnPosition());

        GameState fineState;
        Player finePlayer(1.0f, 1.0f, fineState);
        Enemy fineEnemy(Type::Red, grid, finePlayer, grid.getRedGhostSpawnPosition());

        markAsReleased(coarseEnemy, grid);
        markAsReleased(fineEnemy, grid);

        coarseEnemy.set_position({2.0f, 2.0f});
        coarseEnemy.calc_direction({2.0f, 2.0f}, {3.0f, 2.0f});
        fineEnemy.set_position({2.0f, 2.0f});
        fineEnemy.calc_direction({2.0f, 2.0f}, {3.0f, 2.0f});

        constexpr float coarseDelta{1.0f / 3.0f};
        constexpr float fineDelta{1.0f / 6.0f};

        for (int step = 1; step <= 2; ++step)
        {
            coarseEnemy.update(static_cast<float>(step) * coarseDelta, 1, coarseDelta);
        }

        for (int step = 1; step <= 4; ++step)
        {
            fineEnemy.update(static_cast<float>(step) * fineDelta, 1, fineDelta);
        }

        CHECK(isNear(coarseEnemy.get_position(), {4.0f, 2.0f}));
        CHECK(isNear(fineEnemy.get_position(), coarseEnemy.get_position()));
    }

    TEST_CASE("Ghost reverses safely when a dead end has no other move",
              "[enemy][dead-end][decision]")
    {
        TemporaryEnemyLevel level;
        Grid grid;
        REQUIRE(grid.loadFromFile(level.getPath()));

        GameState gameState;
        Player player(1.0f, 1.0f, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        markAsReleased(enemy, grid);

        enemy.set_position({7.0f, 1.0f});
        enemy.calc_direction({7.0f, 1.0f}, {8.0f, 1.0f});
        enemy.update(1.0f, 1, 1.0f / 3.0f);

        CHECK(isNear(enemy.get_position(), {6.0f, 1.0f}));
    }

    TEST_CASE("Spawn gate blocks living ghosts but allows Dead ghosts",
              "[enemy][spawn][gate][dead]")
    {
        TemporaryEnemyLevel level;
        Grid grid;
        REQUIRE(grid.loadFromFile(level.getPath()));

        GameState gameState;
        Player player(1.0f, 1.0f, gameState);
        Enemy enemy(Type::Red, grid, player, grid.getRedGhostSpawnPosition());

        markAsReleased(enemy, grid);

        const glm::vec2 entrance = grid.getGhostEntryPositions().front();
        const glm::vec2 exit = grid.getGhostExitPositions().front();

        enemy.set_position(exit);
        enemy.calc_direction(exit, entrance);

        SECTION("living ghost")
        {
            enemy.update(1.0f, 1, 1.0f / 3.0f);

            CHECK_FALSE(isNear(enemy.get_position(), entrance));
        }

        SECTION("Dead ghost")
        {
            enemy.set_state_dead();
            enemy.update(1.0f, 1, 1.0f / 3.0f);

            CHECK(isNear(enemy.get_position(), entrance));
        }
    }

    TEST_CASE("Game operations affect and reset every ghost", "[game][enemy][reset]")
    {
        Game game({LEVEL_PATH});
        REQUIRE(game.isInitialized());
        REQUIRE(game.startNewGame(0.0f));

        auto initialEnemies = game.getEnemies();
        for (const auto& enemy : initialEnemies)
        {
            REQUIRE(enemy.get().get_state() == State::Scatter);
        }

        Player& player = const_cast<Player&>(game.getPlayer());
        player.setEnergizerTrue();
        game.update(2.0f, 0.0f);

        for (const auto& enemy : game.getEnemies())
        {
            REQUIRE(enemy.get().get_state() == State::Scared);
        }

        game.resetRound(2.0f);
        const std::array<glm::vec2, 4> spawns{game.getGrid().getRedGhostSpawnPosition(),
                                              game.getGrid().getPinkGhostSpawnPosition(),
                                              game.getGrid().getBlueGhostSpawnPosition(),
                                              game.getGrid().getOrangeGhostSpawnPosition()};
        const auto resetEnemies = game.getEnemies();

        for (std::size_t index = 0; index < resetEnemies.size(); ++index)
        {
            REQUIRE(resetEnemies[index].get().get_state() == State::Scatter);
            REQUIRE(isNear(resetEnemies[index].get().get_position(), spawns[index]));
        }
    }

    // NOLINTEND(bugprone-chained-comparison)
} // namespace
