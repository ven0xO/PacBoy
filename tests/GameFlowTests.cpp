#include "Enemy.hpp"
#include "Game.hpp"
#include "GameInput.hpp"
#include "GameState.hpp"
#include "Player.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{
    constexpr float READY_DURATION{2.0f};
    constexpr float LEVEL_COMPLETE_DURATION{2.5f};

    class TemporaryLevel
    {
    public:
        explicit TemporaryLevel(const std::string& contents)
        {
            static unsigned int fileIndex{0};
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

            path = std::filesystem::temp_directory_path() /
                   ("pacboy_game_flow_" + std::to_string(timestamp) + "_" +
                    std::to_string(fileIndex++) + ".txt");

            std::ofstream file(path);
            file << contents;

            if (!file)
            {
                throw std::runtime_error("Failed to create a temporary level file");
            }
        }

        ~TemporaryLevel()
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }

        TemporaryLevel(const TemporaryLevel&) = delete;
        TemporaryLevel& operator=(const TemporaryLevel&) = delete;

        std::string getPath() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    GameInput pressedEnter()
    {
        GameInput input;
        input.enter = true;
        return input;
    }

    GameInput pressedPause()
    {
        GameInput input;
        input.pause = true;
        return input;
    }

    GameInput pressedForward()
    {
        GameInput input;
        input.up = true;
        return input;
    }

    void releaseKeys(Game& game, float currentFrame)
    {
        game.processPlayerInput(GameInput{}, currentFrame);
    }
} // namespace

// Catch2 intentionally decomposes assertions through an overloaded <= operator.
// NOLINTBEGIN(bugprone-chained-comparison)

TEST_CASE("Ghost score multiplier escalates and resets for a new energizer")
{
    GameState state;
    Player player(0.0f, 0.0f, state);

    player.setEnergizerTrue();
    player.killGhost();
    CHECK(state.getScore() == 200);

    player.killGhost();
    CHECK(state.getScore() == 600);

    player.killGhost();
    CHECK(state.getScore() == 1400);

    player.killGhost();
    CHECK(state.getScore() == 3000);

    player.killGhost();
    CHECK(state.getScore() == 4600);

    player.setEnergizerTrue();
    player.killGhost();
    CHECK(state.getScore() == 4800);
}

TEST_CASE("Dangerous collisions respect invulnerability and the final hit ends the game")
{
    const TemporaryLevel level{"########\n"
                               "#rPSE  #\n"
                               "#pbo   #\n"
                               "########\n"};

    Game game({level.getPath()});
    REQUIRE(game.isInitialized());
    REQUIRE(game.startNewGame(0.0f));

    game.update(READY_DURATION, 0.1f);

    REQUIRE(game.getGameState().getLives() == 2);
    REQUIRE(game.getPhase() == GamePhase::Ready);

    game.update(2.0f * READY_DURATION, 0.1f);

    CHECK(game.getGameState().getLives() == 2);
    CHECK(game.getPhase() == GamePhase::Playing);

    game.update(2.0f * READY_DURATION + 0.1f, 1.4f);

    REQUIRE(game.getGameState().getLives() == 1);
    REQUIRE(game.getPhase() == GamePhase::Ready);

    game.update(3.0f * READY_DURATION + 0.1f, 1.5f);

    CHECK(game.getGameState().getLives() == 0);
    CHECK(game.getGameState().isGameOver());
    CHECK(game.getPhase() == GamePhase::GameOver);

    REQUIRE(game.startNewGame(10.0f));

    CHECK(game.getGameState().getScore() == 0);
    CHECK(game.getGameState().getLives() == 3);
    CHECK(game.getGameState().getLevel() == 1);
    CHECK_FALSE(game.getGameState().isGameOver());
    CHECK(game.getPhase() == GamePhase::Ready);
    CHECK(game.getPlayer().getPosition() == game.getGrid().getPacmanStartPosition());

    for (const Enemy& enemy : game.getEnemies())
    {
        CHECK(enemy.get_state() == State::Scatter);
        CHECK(enemy.get_position() == enemy.get_spawn_point());
    }
}

TEST_CASE("Eating an energizer makes a colliding ghost scared and awards points")
{
    const TemporaryLevel level{"#########\n"
                               "#P*rSE  #\n"
                               "#pbo#####\n"
                               "#########\n"};

    Game game({level.getPath()});
    REQUIRE(game.isInitialized());
    REQUIRE(game.startNewGame(0.0f));

    game.processPlayerInput(pressedForward(), 0.0f);
    game.update(READY_DURATION, 0.0f);
    game.update(READY_DURATION + 0.1f, 1.0f / 3.0f);
    game.update(READY_DURATION + 0.2f, 0.0f);

    REQUIRE(game.getGameState().getScore() == 50);

    for (const Enemy& enemy : game.getEnemies())
    {
        REQUIRE(enemy.get_state() == State::Scared);
    }

    game.update(READY_DURATION + 0.3f, 1.0f / 3.0f);

    CHECK(game.getGameState().getScore() == 250);
    CHECK(game.getGameState().getLives() == 3);
    CHECK(game.getPhase() == GamePhase::Playing);
    CHECK(game.getEnemies()[0].get().get_state() == State::Dead);
}

TEST_CASE("Completing a level loads the next map and loops the configured level list")
{
    const TemporaryLevel firstLevel{"########\n"
                                    "#P     #\n"
                                    "#rSE   #\n"
                                    "#pbo   #\n"
                                    "########\n"};
    const TemporaryLevel secondLevel{"##########\n"
                                     "#   P    #\n"
                                     "#  rSE   #\n"
                                     "#  pbo   #\n"
                                     "##########\n"};

    Game game({firstLevel.getPath(), secondLevel.getPath()});
    REQUIRE(game.isInitialized());
    REQUIRE(game.startNewGame(0.0f));

    game.update(READY_DURATION, 0.0f);
    REQUIRE(game.getPhase() == GamePhase::Playing);

    game.nextLevel(READY_DURATION);
    REQUIRE(game.getPhase() == GamePhase::LevelComplete);
    CHECK(game.getGameState().getLevel() == 1);

    game.nextLevel(READY_DURATION + LEVEL_COMPLETE_DURATION);

    REQUIRE(game.getPhase() == GamePhase::Ready);
    CHECK(game.getGameState().getLevel() == 2);
    CHECK(game.getGrid().getWidth() == 10);
    CHECK(game.getPlayer().getPosition() == game.getGrid().getPacmanStartPosition());

    game.update(2.0f * READY_DURATION + LEVEL_COMPLETE_DURATION, 0.0f);
    REQUIRE(game.getPhase() == GamePhase::Playing);

    game.nextLevel(2.0f * READY_DURATION + LEVEL_COMPLETE_DURATION);
    REQUIRE(game.getPhase() == GamePhase::LevelComplete);

    game.nextLevel(2.0f * READY_DURATION + 2.0f * LEVEL_COMPLETE_DURATION);

    CHECK(game.getPhase() == GamePhase::Ready);
    CHECK(game.getGameState().getLevel() == 3);
    CHECK(game.getGrid().getWidth() == 8);
    CHECK(game.getPlayer().getPosition() == game.getGrid().getPacmanStartPosition());
}

TEST_CASE("Headless gameplay flow covers menu, Ready, Playing, Pause, and level completion")
{
    const TemporaryLevel firstLevel{"########\n"
                                    "#P     #\n"
                                    "#rSE   #\n"
                                    "#pbo   #\n"
                                    "########\n"};
    const TemporaryLevel secondLevel{"##########\n"
                                     "# P      #\n"
                                     "# rSE    #\n"
                                     "# pbo    #\n"
                                     "##########\n"};

    Game game({firstLevel.getPath(), secondLevel.getPath()});
    REQUIRE(game.isInitialized());
    REQUIRE(game.getPhase() == GamePhase::MainMenu);

    game.processPlayerInput(pressedEnter(), 0.0f);
    REQUIRE(game.getPhase() == GamePhase::Ready);

    game.update(READY_DURATION - 0.01f, 0.0f);
    REQUIRE(game.getPhase() == GamePhase::Ready);

    game.update(READY_DURATION, 0.0f);
    REQUIRE(game.getPhase() == GamePhase::Playing);

    game.processPlayerInput(pressedPause(), READY_DURATION);
    REQUIRE(game.getPhase() == GamePhase::Paused);

    releaseKeys(game, READY_DURATION);
    game.processPlayerInput(pressedPause(), READY_DURATION);
    REQUIRE(game.getPhase() == GamePhase::Playing);

    releaseKeys(game, READY_DURATION);
    game.nextLevel(READY_DURATION);
    REQUIRE(game.getPhase() == GamePhase::LevelComplete);

    game.processPlayerInput(pressedEnter(), READY_DURATION + 0.1f);
    REQUIRE(game.getPhase() == GamePhase::Ready);
    CHECK(game.getGameState().getLevel() == 2);

    game.update(2.0f * READY_DURATION + 0.1f, 0.0f);
    CHECK(game.getPhase() == GamePhase::Playing);
}

// NOLINTEND(bugprone-chained-comparison)
