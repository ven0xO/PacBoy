#pragma once
#include <vector>
#include <string>
#include<glm/glm.hpp>

// Forward declaration
class Shader;

enum class Tile {
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
    void render(Shader& shader, unsigned int cubeVAO);
    void collectTile(int x, int y);
    int getWidth() const;
    int getHeight() const;
    int getInitEnergizerCount() const { return initEnergizerCount; }
    int getInitPelletCount() const { return initPelletCount; }
    glm::vec2 getPacmanStartPosition() const;
    glm::vec2 getGhostSpawnPosition() const;
    glm::vec2 getGhostEntryPosition() const {return ghostEntrancePos;}
    glm::vec2 getGhostExitPosition() const {return ghostExitPos;}
    std::vector<glm::vec2> possible_moves(glm::vec2 position);
    glm::vec2 wrapPosition(glm::vec2 position) const;
private:
    std::vector<Tile> tiles;
    int width;
    int height;
    int initPelletCount{0};
    int initEnergizerCount{0};
    glm::vec2 pacmanStartPos;
    glm::vec2 ghostStartPos;
    glm::vec2 ghostEntrancePos;
    glm::vec2 ghostExitPos;
    std::vector<std::string> lines;
};