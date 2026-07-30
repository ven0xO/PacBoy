#include "GameState.hpp"
#include "Scoreboard.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    class TemporaryScoreFile
    {
    public:
        TemporaryScoreFile()
        {
            static std::atomic<unsigned long> nextId{0};

            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            directory = std::filesystem::temp_directory_path() /
                        ("pacboy-scoreboard-tests-" + std::to_string(timestamp) + "-" +
                         std::to_string(nextId++));
            file = directory / "scores.json";

            std::filesystem::create_directories(directory);
        }

        ~TemporaryScoreFile()
        {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }

        TemporaryScoreFile(const TemporaryScoreFile&) = delete;
        TemporaryScoreFile& operator=(const TemporaryScoreFile&) = delete;

        const std::filesystem::path& path() const
        {
            return file;
        }

        void write(const std::string& contents) const
        {
            std::ofstream output(file);

            if (!output.is_open())
            {
                throw std::runtime_error("Cannot create temporary scoreboard file");
            }

            output << contents;
        }

    private:
        std::filesystem::path directory;
        std::filesystem::path file;
    };

    std::vector<int> scoresOf(const Scoreboard& scoreboard)
    {
        std::vector<int> scores;

        for (const auto& entry : scoreboard.getScoreEntries())
        {
            scores.push_back(entry.score);
        }

        return scores;
    }
} // namespace

// Catch2 decomposes assertions through an overloaded <= operator. Clang-Tidy sees the expanded
// macro as a chained comparison even though Catch2 handles it intentionally.
// NOLINTBEGIN(bugprone-chained-comparison)

TEST_CASE("GameState reset restores a fresh game", "[game-state]")
{
    GameState state;
    state.addScore(1230);
    state.nextLevel();
    state.nextLevel();
    state.loseLife();
    state.loseLife();
    state.loseLife();

    REQUIRE(state.getScore() == 1230);
    REQUIRE(state.getLevel() == 3);
    REQUIRE(state.isGameOver());

    state.resetState();

    CHECK(state.getScore() == 0);
    CHECK(state.getLives() == 3);
    CHECK(state.getLevel() == 1);
    CHECK_FALSE(state.isGameOver());
}

TEST_CASE("GameState loses one life at a time and enters GameOver at zero", "[game-state]")
{
    GameState state;

    state.loseLife();
    CHECK(state.getLives() == 2);
    CHECK_FALSE(state.isGameOver());

    state.loseLife();
    CHECK(state.getLives() == 1);
    CHECK_FALSE(state.isGameOver());

    state.loseLife();
    CHECK(state.getLives() == 0);
    CHECK(state.isGameOver());

    state.loseLife();
    CHECK(state.getLives() == 0);
    CHECK(state.isGameOver());
}

TEST_CASE("GameState advances levels without resetting progress", "[game-state]")
{
    GameState state;
    state.addScore(500);
    state.loseLife();

    state.nextLevel();
    CHECK(state.getLevel() == 2);
    CHECK(state.getScore() == 500);
    CHECK(state.getLives() == 2);

    state.nextLevel();
    CHECK(state.getLevel() == 3);
}

TEST_CASE("GameState reports level completion after all collectibles are collected", "[game-state]")
{
    GameState state;
    state.setPelletCount(2);
    state.setEnergizerCount(1);

    CHECK_FALSE(state.checkIfNextLevel());

    state.collectPellet();
    state.collectPellet();
    CHECK_FALSE(state.checkIfNextLevel());

    state.collectEnergizer();
    CHECK(state.checkIfNextLevel());
}

TEST_CASE("Scoreboard keeps entries sorted from highest to lowest", "[scoreboard]")
{
    TemporaryScoreFile temporaryFile;
    Scoreboard scoreboard(temporaryFile.path().string());

    scoreboard.addScore("ALICE", 200);
    scoreboard.addScore("BOB", 900);
    scoreboard.addScore("CAROL", 450);

    REQUIRE(scoresOf(scoreboard) == std::vector<int>{900, 450, 200});
    CHECK(scoreboard.getHighScore() == 900);
}

TEST_CASE("Scoreboard keeps only the ten highest entries", "[scoreboard]")
{
    TemporaryScoreFile temporaryFile;
    Scoreboard scoreboard(temporaryFile.path().string());

    for (int index = 0; index < 12; ++index)
    {
        scoreboard.addScore("PLAYER" + std::to_string(index), index * 10);
    }

    const auto& entries = scoreboard.getScoreEntries();
    REQUIRE(entries.size() == 10);
    CHECK(std::is_sorted(entries.begin(), entries.end(),
                         [](const ScoreEntry& first, const ScoreEntry& second)
                         { return first.score > second.score; }));
    CHECK(entries.front().score == 110);
    CHECK(entries.back().score == 20);

    CHECK_FALSE(scoreboard.isHighScore(20));
    CHECK(scoreboard.isHighScore(21));
}

TEST_CASE("Scoreboard accepts every score until it contains ten entries", "[scoreboard]")
{
    TemporaryScoreFile temporaryFile;
    Scoreboard scoreboard(temporaryFile.path().string());

    CHECK(scoreboard.isHighScore(-100));

    for (int index = 0; index < 9; ++index)
    {
        scoreboard.addScore("PLAYER" + std::to_string(index), index);
    }

    CHECK(scoreboard.isHighScore(-100));
}

TEST_CASE("Scoreboard creates a missing file with an empty score list", "[scoreboard][json]")
{
    TemporaryScoreFile temporaryFile;
    REQUIRE_FALSE(std::filesystem::exists(temporaryFile.path()));

    Scoreboard scoreboard(temporaryFile.path().string());

    CHECK(std::filesystem::exists(temporaryFile.path()));
    CHECK(scoreboard.getScoreEntries().empty());
    CHECK(scoreboard.getHighScore() == -1);

    Scoreboard reloaded(temporaryFile.path().string());
    CHECK(reloaded.getScoreEntries().empty());
}

TEST_CASE("Scoreboard handles invalid files as an empty scoreboard", "[scoreboard][json]")
{
    TemporaryScoreFile temporaryFile;

    SECTION("empty file")
    {
        temporaryFile.write("");
    }

    SECTION("malformed JSON")
    {
        temporaryFile.write(R"({"scores": [)");
    }

    SECTION("missing scores field")
    {
        temporaryFile.write(R"({"other": []})");
    }

    SECTION("incomplete score entry")
    {
        temporaryFile.write(
            R"({"scores": [{"name": "VALID", "score": 100}, {"name": "INCOMPLETE"}]})");
    }

    Scoreboard scoreboard(temporaryFile.path().string());

    CHECK(scoreboard.getScoreEntries().empty());
    CHECK(scoreboard.getHighScore() == -1);
}

TEST_CASE("Scoreboard saves entries that can be loaded again", "[scoreboard][json]")
{
    TemporaryScoreFile temporaryFile;

    {
        Scoreboard scoreboard(temporaryFile.path().string());
        scoreboard.addScore("LOW", 50);
        scoreboard.addScore("HIGH", 750);
        scoreboard.addScore("MIDDLE", 300);
    }

    Scoreboard reloaded(temporaryFile.path().string());
    const auto& entries = reloaded.getScoreEntries();

    REQUIRE(entries.size() == 3);
    CHECK(entries[0].name == "HIGH");
    CHECK(entries[0].score == 750);
    CHECK(entries[1].name == "MIDDLE");
    CHECK(entries[1].score == 300);
    CHECK(entries[2].name == "LOW");
    CHECK(entries[2].score == 50);
}

// NOLINTEND(bugprone-chained-comparison)
