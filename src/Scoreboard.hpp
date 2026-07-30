#pragma once

#include <string>
#include <vector>

struct ScoreEntry
{
    std::string name;
    int score{0};
};

class Scoreboard
{
public:
    explicit Scoreboard(const std::string& path);
    int getHighScore() const
    {
        return entries.empty() ? -1 : entries[0].score;
    }
    void addScore(const std::string& name, int score);
    bool isHighScore(int score) const;
    const std::vector<ScoreEntry>& getScoreEntries() const
    {
        return entries;
    }

private:
    void loadFromFile();
    void saveToFile() const;

    std::vector<ScoreEntry> entries;
    std::string filePath;
};
