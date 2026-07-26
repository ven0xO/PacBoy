#pragma once

#include <string>
#include "../external/shader_s.h"
#include <glm/glm.hpp>

class GameState;
class Shader;

class Hud
{
public:
    Hud(int widthi, int heighti);

    void render(const GameState& state);
    void renderMainMenu(int selected);

private:
    void renderText(
        const std::string& text,
        float x,
        float y,
        float scale
    );
    void drawRectangle(
        float x,
        float y,
        float rectangleWidth,
        float rectangleHeight,
        glm::vec3 color
    );

    unsigned int VAO;
    unsigned int VBO;
    Shader shader;
    int width;
    int height;
};