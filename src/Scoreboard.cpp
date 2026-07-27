#include "Scoreboard.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <nlohmann/json.hpp>

Scoreboard::Scoreboard(const std::string& path)
    : filePath(path)
{
    loadFromFile();
    std::sort(
        entries.begin(),
        entries.end(),
        [](const ScoreEntry& first, const ScoreEntry& second)
        {
            return first.score > second.score;
        }
    );

    if (entries.size() > 10)
    {
        entries.resize(10);
    }
}

void Scoreboard::addScore(const std::string& name, int score)
{
    entries.push_back({name, score});

    std::sort(
        entries.begin(),
        entries.end(),
        [](const ScoreEntry& first, const ScoreEntry& second)
        {
            return first.score > second.score;
        }
    );

    if (entries.size() > 10)
    {
        entries.resize(10);
    }

    saveToFile();
}

bool Scoreboard::isHighScore(int score) const
{
    return (entries.size() < 10 || score > entries[9].score);
}

void Scoreboard::loadFromFile()
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        std::filesystem::path directory = std::filesystem::path(filePath).parent_path();

        std::error_code error;

        if (!directory.empty())
        {
            std::filesystem::create_directories(directory, error);
        }

        if (error)
        {
            std::cerr
                << "Cannot create scoreboard directory: "
                << error.message() << '\n';
            return;
        }

        saveToFile();
        return;
    }

    try
    {
        nlohmann::json data;
        file >> data;

        std::vector<ScoreEntry> loadedEntries;

        for(const auto& scoreData : data.at("scores"))
        {
            loadedEntries.push_back({
                scoreData.at("name").get<std::string>(),
                scoreData.at("score").get<int>()
            });
        }

        entries = loadedEntries;
    }
    catch (const nlohmann::json::exception& error)
    {
        entries.clear();

        std::cerr
            << "Failed to load scoreboard '" << filePath
            << "': " << error.what()
            << "; using an empty scoreboard.\n";
    }
}

void Scoreboard::saveToFile() const
{
    nlohmann::json data;
    data["scores"] = nlohmann::json::array();

    for (const auto& entry : entries)
    {
        data["scores"].push_back({
            {"name", entry.name},
            {"score", entry.score}
        });
    }

    std::ofstream file(filePath);

    if (!file.is_open())
    {
        std::cerr << "Cannot save scoreboard file: " << filePath << "\n";
        return;
    }

    file << data.dump(4);
}
