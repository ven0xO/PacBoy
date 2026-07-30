#pragma once
#include <vector>
#include <string>
#include <array>
#include <glm/glm.hpp>

enum class Tile
{
    Empty,
    Wall,
    Pellet,
    Energizer,
    PacmanStart,
    GhostStart,
    GhostSpawnEntrance,
    GhostSpawnExit,
    Tunnel
};

class Grid
{
public:
    bool loadFromFile(const std::string& path);
    Tile getTile(int x, int y) const;
    void collectTile(int x, int y);
    int getWidth() const;
    int getHeight() const;
    int getInitEnergizerCount() const
    {
        return initEnergizerCount;
    }
    int getInitPelletCount() const
    {
        return initPelletCount;
    }
    glm::vec2 getPacmanStartPosition() const;
    std::vector<glm::vec2> possible_moves(glm::vec2 position);
    glm::vec2 wrapPosition(glm::vec2 position) const;

    glm::vec2 getRedGhostSpawnPosition() const
    {
        return ghostStartPositions[0];
    }
    glm::vec2 getPinkGhostSpawnPosition() const
    {
        return ghostStartPositions[1];
    }
    glm::vec2 getBlueGhostSpawnPosition() const
    {
        return ghostStartPositions[2];
    }
    glm::vec2 getOrangeGhostSpawnPosition() const
    {
        return ghostStartPositions[3];
    }
    const std::vector<glm::vec2>& getGhostEntryPositions() const
    {
        return ghostEntrancePositions;
    }
    const std::vector<glm::vec2>& getGhostExitPositions() const
    {
        return ghostExitPositions;
    }

private:
    std::vector<Tile> tiles;
    int width{0};
    int height{0};
    int initPelletCount{0};
    int initEnergizerCount{0};
    glm::vec2 pacmanStartPos{0.0f};
    std::array<glm::vec2, 4> ghostStartPositions{};
    std::vector<glm::vec2> ghostEntrancePositions;
    std::vector<glm::vec2> ghostExitPositions;
};
