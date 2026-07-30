#include "GameState.hpp"
#include "Grid.hpp"
#include "Player.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
    constexpr std::string_view VALID_LEVEL{"#######\n"
                                           "#P..r #\n"
                                           "#pSEbo#\n"
                                           "#.*...#\n"
                                           "#######\n"};

    constexpr std::string_view MOVEMENT_LEVEL{"#########\n"
                                              "#r.....p#\n"
                                              "#.P.....#\n"
                                              "#.......#\n"
                                              "#b.SE.o.#\n"
                                              "#.......#\n"
                                              "#########\n"};

    class TemporaryLevel
    {
    public:
        explicit TemporaryLevel(std::string_view contents)
        {
            static std::atomic<unsigned long> nextId{0};

            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            directory =
                std::filesystem::temp_directory_path() /
                ("pacboy-grid-tests-" + std::to_string(timestamp) + "-" + std::to_string(nextId++));
            file = directory / "level.txt";

            std::error_code error;
            if (!std::filesystem::create_directories(directory, error) || error)
            {
                throw std::runtime_error("Cannot create temporary level directory");
            }

            std::ofstream output(file);
            if (!output.is_open())
            {
                std::filesystem::remove_all(directory, error);
                throw std::runtime_error("Cannot create temporary level file");
            }

            output << contents;
            if (!output)
            {
                output.close();
                std::filesystem::remove_all(directory, error);
                throw std::runtime_error("Cannot write temporary level file");
            }
        }

        ~TemporaryLevel()
        {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }

        TemporaryLevel(const TemporaryLevel&) = delete;
        TemporaryLevel& operator=(const TemporaryLevel&) = delete;

        std::string path() const
        {
            return file.string();
        }

    private:
        std::filesystem::path directory;
        std::filesystem::path file;
    };

    bool isNear(const glm::vec2& first, const glm::vec2& second)
    {
        constexpr float EPSILON{0.0001f};
        return glm::length(first - second) < EPSILON;
    }
} // namespace

// Catch2 decomposes assertions through overloaded comparison operators. Clang-Tidy sees the
// expanded macros as chained comparisons even though Catch2 handles them intentionally.
// NOLINTBEGIN(bugprone-chained-comparison)

TEST_CASE("Grid parses a valid level and records its markers", "[grid][map]")
{
    TemporaryLevel level(VALID_LEVEL);
    Grid grid;

    REQUIRE(grid.loadFromFile(level.path()));

    CHECK(grid.getWidth() == 7);
    CHECK(grid.getHeight() == 5);
    CHECK(isNear(grid.getPacmanStartPosition(), {1.0f, 1.0f}));
    CHECK(isNear(grid.getRedGhostSpawnPosition(), {4.0f, 1.0f}));
    CHECK(isNear(grid.getPinkGhostSpawnPosition(), {1.0f, 2.0f}));
    CHECK(isNear(grid.getBlueGhostSpawnPosition(), {4.0f, 2.0f}));
    CHECK(isNear(grid.getOrangeGhostSpawnPosition(), {5.0f, 2.0f}));

    REQUIRE(grid.getGhostEntryPositions().size() == 1);
    REQUIRE(grid.getGhostExitPositions().size() == 1);
    CHECK(isNear(grid.getGhostEntryPositions().front(), {2.0f, 2.0f}));
    CHECK(isNear(grid.getGhostExitPositions().front(), {3.0f, 2.0f}));

    CHECK(grid.getTile(0, 0) == Tile::Wall);
    CHECK(grid.getTile(1, 1) == Tile::PacmanStart);
    CHECK(grid.getTile(2, 1) == Tile::Pellet);
    CHECK(grid.getTile(5, 1) == Tile::Empty);
    CHECK(grid.getTile(2, 3) == Tile::Energizer);
}

TEST_CASE("Grid rejects invalid level data", "[grid][map][validation]")
{
    Grid grid;

    SECTION("empty file")
    {
        TemporaryLevel level("");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("unsupported character")
    {
        TemporaryLevel level("#######\n"
                             "#P..r##\n"
                             "#pSXbo#\n"
                             "#.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("missing player marker")
    {
        TemporaryLevel level("#######\n"
                             "#...r##\n"
                             "#pSEbo#\n"
                             "#.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("duplicate ghost marker")
    {
        TemporaryLevel level("#######\n"
                             "#P.rr##\n"
                             "#pSEbo#\n"
                             "#.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("different entrance and exit counts")
    {
        TemporaryLevel level("#######\n"
                             "#P..r##\n"
                             "#pS.bo#\n"
                             "#.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("entrance and exit are not adjacent")
    {
        TemporaryLevel level("#######\n"
                             "#P..r##\n"
                             "#pSboE#\n"
                             "#.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }

    SECTION("tunnel has no endpoint on the opposite edge")
    {
        TemporaryLevel level("#######\n"
                             "#P..r##\n"
                             "#pSEbo#\n"
                             "T.*...#\n"
                             "#######\n");
        CHECK_FALSE(grid.loadFromFile(level.path()));
    }
}

TEST_CASE("Failed parsing keeps the current valid grid unchanged", "[grid][map][validation]")
{
    TemporaryLevel validLevel(VALID_LEVEL);
    TemporaryLevel invalidLevel("#######\n"
                                "#P..r##\n"
                                "#pSXbo#\n"
                                "#.*...#\n"
                                "#######\n");
    Grid grid;

    REQUIRE(grid.loadFromFile(validLevel.path()));
    REQUIRE_FALSE(grid.loadFromFile(invalidLevel.path()));

    CHECK(grid.getWidth() == 7);
    CHECK(grid.getHeight() == 5);
    CHECK(grid.getTile(2, 1) == Tile::Pellet);
    CHECK(isNear(grid.getPacmanStartPosition(), {1.0f, 1.0f}));
}

TEST_CASE("Grid treats outside positions as walls except at paired tunnels",
          "[grid][bounds][tunnel]")
{
    TemporaryLevel level("##T####\n"
                         "#P..r##\n"
                         "#pSEbo#\n"
                         "T.*...T\n"
                         "##T####\n");
    Grid grid;

    REQUIRE(grid.loadFromFile(level.path()));

    CHECK(grid.getTile(-1, 1) == Tile::Wall);
    CHECK(grid.getTile(grid.getWidth(), 1) == Tile::Wall);
    CHECK(grid.getTile(1, -1) == Tile::Wall);
    CHECK(grid.getTile(1, grid.getHeight()) == Tile::Wall);
    CHECK(grid.getTile(-2, 3) == Tile::Wall);

    CHECK(grid.getTile(-1, 3) == Tile::Tunnel);
    CHECK(grid.getTile(grid.getWidth(), 3) == Tile::Tunnel);
    CHECK(grid.getTile(2, -1) == Tile::Tunnel);
    CHECK(grid.getTile(2, grid.getHeight()) == Tile::Tunnel);

    CHECK(isNear(grid.wrapPosition({-1.0f, 3.0f}), {6.0f, 3.0f}));
    CHECK(isNear(grid.wrapPosition({7.0f, 3.0f}), {0.0f, 3.0f}));
    CHECK(isNear(grid.wrapPosition({2.0f, -1.0f}), {2.0f, 4.0f}));
    CHECK(isNear(grid.wrapPosition({2.0f, 5.0f}), {2.0f, 0.0f}));
}

TEST_CASE("Grid counts collectibles and removes collected tiles", "[grid][collectibles]")
{
    TemporaryLevel level(VALID_LEVEL);
    Grid grid;

    REQUIRE(grid.loadFromFile(level.path()));
    REQUIRE(grid.getInitPelletCount() == 6);
    REQUIRE(grid.getInitEnergizerCount() == 1);

    grid.collectTile(2, 1);
    grid.collectTile(2, 3);

    CHECK(grid.getTile(2, 1) == Tile::Empty);
    CHECK(grid.getTile(2, 3) == Tile::Empty);
    CHECK(grid.getInitPelletCount() == 6);
    CHECK(grid.getInitEnergizerCount() == 1);
}

TEST_CASE("Player movement covers the same distance with smaller deltaTime steps",
          "[player][movement][delta-time]")
{
    TemporaryLevel level(MOVEMENT_LEVEL);
    Grid grid;
    REQUIRE(grid.loadFromFile(level.path()));

    GameState coarseState;
    Player coarsePlayer(2.0f, 2.0f, coarseState);
    coarsePlayer.update(grid, 0.1f);

    GameState fineState;
    Player finePlayer(2.0f, 2.0f, fineState);
    finePlayer.update(grid, 0.05f);
    finePlayer.update(grid, 0.05f);

    GameState finerState;
    Player finerPlayer(2.0f, 2.0f, finerState);
    for (int frame = 0; frame < 4; ++frame)
    {
        finerPlayer.update(grid, 0.025f);
    }

    CHECK(coarsePlayer.getPosition().x == Catch::Approx(2.3f));
    CHECK(coarsePlayer.getPosition().y == Catch::Approx(2.0f));
    CHECK(finePlayer.getPosition().x == Catch::Approx(coarsePlayer.getPosition().x));
    CHECK(finePlayer.getPosition().y == Catch::Approx(coarsePlayer.getPosition().y));
    CHECK(finerPlayer.getPosition().x == Catch::Approx(coarsePlayer.getPosition().x));
    CHECK(finerPlayer.getPosition().y == Catch::Approx(coarsePlayer.getPosition().y));

    constexpr float COARSE_CENTER_STEP{1.0f / 6.0f};
    constexpr float FINE_CENTER_STEP{1.0f / 12.0f};

    GameState coarseCenterState;
    Player coarseCenterPlayer(2.0f, 2.0f, coarseCenterState);
    coarseCenterPlayer.update(grid, COARSE_CENTER_STEP);
    coarseCenterPlayer.update(grid, COARSE_CENTER_STEP);

    GameState fineCenterState;
    Player fineCenterPlayer(2.0f, 2.0f, fineCenterState);
    for (int frame = 0; frame < 4; ++frame)
    {
        fineCenterPlayer.update(grid, FINE_CENTER_STEP);
    }

    CHECK(coarseCenterPlayer.getPosition().x == Catch::Approx(3.0f));
    CHECK(coarseCenterPlayer.getPosition().y == Catch::Approx(2.0f));
    CHECK(fineCenterPlayer.getPosition().x == Catch::Approx(coarseCenterPlayer.getPosition().x));
    CHECK(fineCenterPlayer.getPosition().y == Catch::Approx(coarseCenterPlayer.getPosition().y));
}

TEST_CASE("Player turns immediately at a tile center for different deltaTime values",
          "[player][turning][delta-time]")
{
    TemporaryLevel level(MOVEMENT_LEVEL);
    Grid grid;
    REQUIRE(grid.loadFromFile(level.path()));

    GameState coarseState;
    Player coarsePlayer(2.0f, 2.0f, coarseState);
    coarsePlayer.setDirection(Direction::Right);
    coarsePlayer.update(grid, 0.1f);

    GameState fineState;
    Player finePlayer(2.0f, 2.0f, fineState);
    finePlayer.setDirection(Direction::Right);
    finePlayer.update(grid, 0.05f);
    finePlayer.update(grid, 0.05f);

    CHECK(isNear(coarsePlayer.getCurrentDirection(), {0.0f, 1.0f}));
    CHECK(isNear(finePlayer.getCurrentDirection(), {0.0f, 1.0f}));
    CHECK(coarsePlayer.getPosition().x == Catch::Approx(2.0f));
    CHECK(coarsePlayer.getPosition().y == Catch::Approx(2.3f));
    CHECK(finePlayer.getPosition().x == Catch::Approx(coarsePlayer.getPosition().x));
    CHECK(finePlayer.getPosition().y == Catch::Approx(coarsePlayer.getPosition().y));
}

TEST_CASE("Player queues the same turn across different deltaTime values",
          "[player][turning][delta-time]")
{
    TemporaryLevel level(MOVEMENT_LEVEL);
    Grid grid;
    REQUIRE(grid.loadFromFile(level.path()));

    constexpr float COARSE_STEP{1.0f / 6.0f};
    constexpr float FINE_STEP{1.0f / 12.0f};

    GameState coarseState;
    Player coarsePlayer(2.0f, 2.0f, coarseState);
    coarsePlayer.update(grid, COARSE_STEP);

    GameState fineState;
    Player finePlayer(2.0f, 2.0f, fineState);
    finePlayer.update(grid, FINE_STEP);
    finePlayer.update(grid, FINE_STEP);

    REQUIRE(isNear(coarsePlayer.getPosition(), {2.5f, 2.0f}));
    REQUIRE(isNear(finePlayer.getPosition(), coarsePlayer.getPosition()));

    coarsePlayer.setDirection(Direction::Right);
    finePlayer.setDirection(Direction::Right);

    coarsePlayer.update(grid, COARSE_STEP);
    finePlayer.update(grid, FINE_STEP);
    finePlayer.update(grid, FINE_STEP);

    REQUIRE(isNear(coarsePlayer.getPosition(), {3.0f, 2.0f}));
    REQUIRE(isNear(finePlayer.getPosition(), coarsePlayer.getPosition()));
    REQUIRE(isNear(coarsePlayer.getCurrentDirection(), {1.0f, 0.0f}));
    REQUIRE(isNear(finePlayer.getCurrentDirection(), {1.0f, 0.0f}));

    coarsePlayer.update(grid, COARSE_STEP);
    finePlayer.update(grid, FINE_STEP);
    finePlayer.update(grid, FINE_STEP);

    CHECK(isNear(coarsePlayer.getCurrentDirection(), {0.0f, 1.0f}));
    CHECK(isNear(finePlayer.getCurrentDirection(), coarsePlayer.getCurrentDirection()));
    CHECK(coarsePlayer.getPosition().x == Catch::Approx(3.0f));
    CHECK(coarsePlayer.getPosition().y == Catch::Approx(2.5f));
    CHECK(finePlayer.getPosition().x == Catch::Approx(coarsePlayer.getPosition().x));
    CHECK(finePlayer.getPosition().y == Catch::Approx(coarsePlayer.getPosition().y));
}

// NOLINTEND(bugprone-chained-comparison)
