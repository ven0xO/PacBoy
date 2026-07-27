#include "Scoreboard.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
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

    if(!file.is_open())
    {
        std::filesystem::create_directories(
            std::filesystem::path(filePath).parent_path()
        );
        saveToFile();
        return;
    }

    nlohmann::json data;
    file >> data;

    for (const auto& scoreData : data.at("scores"))
    {
        entries.push_back({
            scoreData.at("name").get<std::string>(),
            scoreData.at("score").get<int>()
        });
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
