#include "Grid.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../external/GLAD/include/glad/glad.h"
#include "../external/shader_s.h"

bool Grid::loadFromFile(const std::string& path)
{
    std::ifstream in(path);
    if(!in.is_open())
    {
        std::cerr << "Cannot open level file: " << path << "\n";
        return false;
    }

    lines.clear();
    tiles.clear();
    ghostEntrancePositions.clear();
    ghostExitPositions.clear();

    initPelletCount = 0;
    initEnergizerCount = 0;

    int playerSpawnCount = 0;
    std::array<int, 4> ghostSpawnCounts{};
    std::vector<glm::ivec2> tunnelPositions;

    std::string line;

    while(std::getline(in, line))
    {
        if(!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    if (lines.empty())
    {
        std::cerr << "Level file is empty: " << path << "\n";
        return false;
    }

    height = lines.size();
    width = lines[0].size();
    
    // Support non-rectangular level files by padding shorter rows.
    int maxWidth = 0;
    for (const auto& l : lines) {
        maxWidth = std::max(maxWidth, (int)l.size());
    }
    tiles.resize(maxWidth * height);

    for(int y{}; y < height; y++)
    {
        int rowWidth = lines[y].size();

        for (int x=0; x<rowWidth; ++x) {
            char c = lines[y][x];
            Tile t = Tile::Empty;

            // Level map characters:
            // '#' - wall
            // '.' - pellet
            // '*' - energizer
            // 'P' - player spawn
            // 'r' - red ghost spawn
            // 'p' - pink ghost spawn
            // 'b' - blue ghost spawn
            // 'o' - orange ghost spawn
            // 'E' - ghost-house exit
            // 'S' - ghost-house entrance/gate
            // 'T' - tunnel endpoint
            // ' ' - empty tile
            switch(c) {
                case '#': t=Tile::Wall; break;
                case '.': t=Tile::Pellet; initPelletCount++; break;
                case '*': t=Tile::Energizer; initEnergizerCount++; break;
                case 'P':
                    t = Tile::PacmanStart;
                    pacmanStartPos = {x, y};
                    playerSpawnCount++;
                    break;
                case 'r':
                    t = Tile::GhostStart;
                    ghostStartPositions[0] = {x, y};
                    ghostSpawnCounts[0]++;
                    break;
                case 'p':
                    t = Tile::GhostStart;
                    ghostStartPositions[1] = {x, y};
                    ghostSpawnCounts[1]++;
                    break;
                case 'b':
                    t = Tile::GhostStart;
                    ghostStartPositions[2] = {x, y};
                    ghostSpawnCounts[2]++;
                    break;
                case 'o':
                    t = Tile::GhostStart;
                    ghostStartPositions[3] = {x, y};
                    ghostSpawnCounts[3]++;
                    break;
                case 'E':
                    t = Tile::GhostSpawnExit;
                    ghostExitPositions.emplace_back(
                        static_cast<float>(x),
                        static_cast<float>(y)
                    );
                    break;

                case 'S':
                    t = Tile::GhostSpawnEntrance;
                    ghostEntrancePositions.emplace_back(
                        static_cast<float>(x),
                        static_cast<float>(y)
                    );
                    break;
                case 'T':
                    t = Tile::Tunnel;
                    tunnelPositions.emplace_back(x, y);
                    break;
                case ' ': t=Tile::Empty; break;
                default:
                    std::cerr
                        << "Unsupported character '" << c
                        << "' in level file " << path
                        << " at (" << x << ", " << y << ").\n";
                    return false;
            }
            tiles[y*maxWidth+x] = t;
        }
        for (int x = rowWidth; x < maxWidth; ++x) {
            tiles[y*maxWidth+x] = Tile::Empty;
        }
    }
    width = maxWidth;

    if (playerSpawnCount != 1)
    {
        std::cerr
            << "Level must contain exactly one P marker: "
            << path << "\n";
        return false;
    }

    constexpr std::array<char, 4> ghostMarkers{'r', 'p', 'b', 'o'};

    for (std::size_t i = 0; i < ghostSpawnCounts.size(); i++)
    {
        if (ghostSpawnCounts[i] != 1)
        {
            std::cerr
                << "Level must contain exactly one "
                << ghostMarkers[i] << " marker: "
                << path << "\n";
            return false;
        }
    }

    if (ghostEntrancePositions.empty())
    {
        std::cerr
            << "Level must contain at least one S marker: "
            << path << "\n";
        return false;
    }

    if (ghostExitPositions.size() != ghostEntrancePositions.size())
    {
        std::cerr
            << "Level must contain the same number of E and S markers: "
            << path << "\n";
        return false;
    }

    const auto areAdjacent = [](
        const glm::vec2& first,
        const glm::vec2& second
    )
    {
        const int deltaX = std::abs(
            static_cast<int>(first.x) -
            static_cast<int>(second.x)
        );
        const int deltaY = std::abs(
            static_cast<int>(first.y) -
            static_cast<int>(second.y)
        );

        return deltaX + deltaY == 1;
    };

    for (const glm::vec2& entrance : ghostEntrancePositions)
    {
        const int adjacentExits = std::count_if(
            ghostExitPositions.begin(),
            ghostExitPositions.end(),
            [&entrance, &areAdjacent](const glm::vec2& exit)
            {
                return areAdjacent(entrance, exit);
            }
        );

        if (adjacentExits != 1)
        {
            std::cerr
                << "Each S marker must have exactly one adjacent E marker: "
                << path << "\n";
            return false;
        }
    }

    for (const glm::vec2& exit : ghostExitPositions)
    {
        const int adjacentEntrances = std::count_if(
            ghostEntrancePositions.begin(),
            ghostEntrancePositions.end(),
            [&exit, &areAdjacent](const glm::vec2& entrance)
            {
                return areAdjacent(exit, entrance);
            }
        );

        if (adjacentEntrances != 1)
        {
            std::cerr
                << "Each E marker must have exactly one adjacent S marker: "
                << path << "\n";
            return false;
        }
    }

    const auto hasTunnelAt = [&tunnelPositions](int x, int y)
    {
        return std::any_of(
            tunnelPositions.begin(),
            tunnelPositions.end(),
            [x, y](const glm::ivec2& tunnel)
            {
                return tunnel.x == x && tunnel.y == y;
            }
        );
    };

    for (const glm::ivec2& tunnel : tunnelPositions)
    {
        const bool onLeft = tunnel.x == 0;
        const bool onRight = tunnel.x == width - 1;
        const bool onTop = tunnel.y == 0;
        const bool onBottom = tunnel.y == height - 1;

        const int edgeCount =
            static_cast<int>(onLeft) +
            static_cast<int>(onRight) +
            static_cast<int>(onTop) +
            static_cast<int>(onBottom);

        if (edgeCount != 1)
        {
            std::cerr
                << "Each T marker must be on one non-corner map edge: "
                << path << "\n";
            return false;
        }

        const bool hasPair =
            (onLeft && hasTunnelAt(width - 1, tunnel.y)) ||
            (onRight && hasTunnelAt(0, tunnel.y)) ||
            (onTop && hasTunnelAt(tunnel.x, height - 1)) ||
            (onBottom && hasTunnelAt(tunnel.x, 0));

        if (!hasPair)
        {
            std::cerr
                << "Each T marker must have a matching marker on the opposite edge: "
                << path << "\n";
            return false;
        }
    }

    return true;
}

int Grid::getWidth() const
{
    return width;
}

int Grid::getHeight() const
{
    return height;
}

Tile Grid::getTile(int x, int y) const
{
    const bool validX = x >= 0 && x < width;
    const bool validY = y >= 0 && y < height;

    if (validX && validY)
    {
        return tiles[y * width + x];
    }

    if (validY)
    {
        if (x == -1 && tiles[y * width] == Tile::Tunnel)
        {
            return Tile::Tunnel;
        }

        if (x == width && tiles[y * width + width - 1] == Tile::Tunnel)
        {
            return Tile::Tunnel;
        }
    }

    if (validX)
    {
        if (y == -1 &&
            tiles[x] == Tile::Tunnel)
        {
            return Tile::Tunnel;
        }

        if (y == height &&
            tiles[(height - 1) * width + x] == Tile::Tunnel)
        {
            return Tile::Tunnel;
        }
    }

    return Tile::Wall;
}

glm::vec2 Grid::getPacmanStartPosition() const
{
    return pacmanStartPos;
}

void Grid::collectTile(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y * width + x] = Tile::Empty;
    }
}

void Grid::render(Shader& shader, unsigned int cubeVAO)
{
    for(int y{}; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            Tile tile = getTile(x,y);
            if (tile == Tile::Empty ||
                tile == Tile::Tunnel ||
                tile == Tile::GhostStart ||
                tile == Tile::GhostSpawnExit ||
                tile == Tile::PacmanStart)
            {
                continue;
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, 0.0f, y));

            int modelLoc = glGetUniformLocation(shader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            int objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
            switch (tile) {
                case Tile::Wall:
                    glUniform3f(objectColorLoc, 0.0f, 0.0f, 1.0f); // blue
                    break;
                case Tile::Pellet:
                    glUniform3f(objectColorLoc, 1.0f, 1.0f, 0.0f); // yellow
                    break;
                case Tile::Energizer:
                    glUniform3f(objectColorLoc, 1.0f, 0.0f, 1.0f); // purple
                    break;
                case Tile::GhostSpawnEntrance:
                    glUniform3f(objectColorLoc, 1.0f, 0.0f, 0.0f); // red
                    break;
            }
            
            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
    }
}

std::vector<glm::vec2> Grid::possible_moves(glm::vec2 position)
{
    // Expected input is a tile-centered position; results are neighboring non-wall tiles.
    std::vector<glm::vec2> result;

    if(getTile(position.x, position.y + 1) != Tile::Wall) result.push_back({position.x, position.y + 1.0f});
    if(getTile(position.x, position.y - 1) != Tile::Wall) result.push_back({position.x, position.y - 1.0f});
    if(getTile(position.x - 1, position.y) != Tile::Wall) result.push_back({position.x - 1.0f, position.y});
    if(getTile(position.x + 1, position.y) != Tile::Wall) result.push_back({position.x + 1.0f, position.y});

    return result;
}

glm::vec2 Grid::wrapPosition(glm::vec2 position) const
{
    const int tileX =
        static_cast<int>(std::round(position.x));

    const int tileY =
        static_cast<int>(std::round(position.y));

    if (position.x <= -1.0f &&
        getTile(-1, tileY) == Tile::Tunnel)
    {
        position.x = static_cast<float>(width - 1);
    }
    else if (position.x >= static_cast<float>(width) &&
             getTile(width, tileY) == Tile::Tunnel)
    {
        position.x = 0.0f;
    }
    else if (position.y <= -1.0f &&
             getTile(tileX, -1) == Tile::Tunnel)
    {
        position.y = static_cast<float>(height - 1);
    }
    else if (position.y >= static_cast<float>(height) &&
             getTile(tileX, height) == Tile::Tunnel)
    {
        position.y = 0.0f;
    }

    return position;
}
