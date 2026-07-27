#include "Hud.hpp"
#include "GameState.hpp"
#include "PixelFont.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

Hud::Hud(int widthi, int heighti) :
shader("./shaders/hud.vs", "./shaders/hud.fs"),
width(widthi),
height(heighti)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        6 * 2 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Hud::render(const GameState& state)
{
    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        38.0f,
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    renderText(
        "SCORE " + std::to_string(state.getScore()),
        12.0f,
        8.0f,
        3.0f
    );

    renderText(
        "LIVES " + std::to_string(state.getLives()),
        300.0f,
        8.0f,
        3.0f
    );

    renderText(
        "LEVEL " + std::to_string(state.getLevel()),
        600.0f,
        8.0f,
        3.0f
    );
}

void Hud::drawRectangle(float x, float y, float rectangleWidth, 
                        float rectangleHeight, glm::vec3 color, 
                        float alpha)
{
    float vertices[] = {
        // first triangle
        x,                  y,
        x + rectangleWidth, y,
        x + rectangleWidth, y + rectangleHeight,

        // second triangle
        x,                  y,
        x + rectangleWidth, y + rectangleHeight,
        x,                  y + rectangleHeight
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f
    );

    shader.use();

    int projectionLocation =
        glGetUniformLocation(shader.ID, "projection");

    glUniformMatrix4fv(
        projectionLocation,
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    int colorLocation = 
        glGetUniformLocation(shader.ID, "hudColor");

    glUniform4f(
        colorLocation,
        color.x,
        color.y,
        color.z,
        alpha
    );

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Hud::renderText(
    const std::string& text,
    float x,
    float y,
    float scale,
    glm::vec3 textColor
)
{
    float cursorX = x;

    for (char character : text)
    {
        const PixelFont::Glyph* glyph = PixelFont::findGlyph(character);

        if (glyph != nullptr)
        {
            for (int row = 0; row < PixelFont::GLYPH_HEIGHT; row++)
            {
                for (int column = 0; column < PixelFont::GLYPH_WIDTH; column++)
                {
                    if ((*glyph)[row][column] == '1')
                    {
                        drawRectangle(
                            cursorX + column * scale,
                            y + row * scale,
                            scale,
                            scale,
                            textColor
                        );
                    }
                }
            }
        }

        cursorX += (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) * scale;
    }
}

void Hud::renderMainMenu(int selected)
{
    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    renderText(
        "PACBOY",
        255.0f,
        120.0f,
        8.0f
    );

    renderText(
        " START GAME",
        255.0f,
        300.0f,
        4.0f
    );

    renderText(
        " SCOREBOARD",
        255.0f,
        360.0f,
        4.0f
    );

    renderText(
        "USE ARROWS AND ENTER",
        220.0f,
        500.0f,
        3.0f
    );

    renderText(
        "PRESS ESC TO EXIT",
        295.0f,
        550.0f,
        2.0f
    );
    
    renderText(
        ">",
        235.0f,
        300.0f + selected * 60,
        4.0f
    );
}

void Hud::renderReady()
{
    const std::string text{"READY?"};
    constexpr float scale{8.0f};
    constexpr float outlineThickness{3.0f};

    const float textWidth =
        (
            static_cast<float>(text.size()) *
            (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
            PixelFont::GLYPH_SPACING
        ) * scale;

    const float textHeight = PixelFont::GLYPH_HEIGHT * scale;
    const float x = (static_cast<float>(width) - textWidth) / 2.0f;
    const float y = (static_cast<float>(height) - textHeight) / 2.0f;

    const glm::vec3 outlineColor(0.0f, 0.0f, 0.0f);

    for (int offsetY = -1; offsetY <= 1; offsetY++)
    {
        for (int offsetX = -1; offsetX <= 1; offsetX++)
        {
            if (offsetX == 0 && offsetY == 0)
            {
                continue;
            }

            renderText(
                text,
                x + offsetX * outlineThickness,
                y + offsetY * outlineThickness,
                scale,
                outlineColor
            );
        }
    }

    renderText(text, x, y, scale);
}

void Hud::renderPause(int selected)
{
    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.35f, 0.35f, 0.35f),
        0.65f
    );

    const std::string title{"PAUSED"};
    const std::string information{"PRESS ESC TO EXIT THE GAME"};
    constexpr float titleScale{10.0f};
    constexpr float optionScale{5.0f};
    constexpr float informationScale{2.0f};
    constexpr float optionsX{275.0f};
    constexpr float firstOptionY{285.0f};
    constexpr float optionSpacing{75.0f};

    const auto centeredX = [this](const std::string& text, float scale)
    {
        const float textWidth =
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;

        return (static_cast<float>(width) - textWidth) / 2.0f;
    };

    renderText(
        title,
        centeredX(title, titleScale),
        100.0f,
        titleScale
    );

    renderText(
        "RESUME",
        optionsX,
        firstOptionY,
        optionScale
    );

    renderText(
        "MAIN MENU",
        optionsX,
        firstOptionY + optionSpacing,
        optionScale
    );

    renderText(
        ">",
        optionsX - 40.0f,
        firstOptionY + selected * optionSpacing,
        optionScale
    );

    renderText(
        information,
        centeredX(information, informationScale),
        550.0f,
        informationScale,
        glm::vec3(0.85f, 0.85f, 0.85f)
    );
}
