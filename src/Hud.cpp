#include "Hud.hpp"
#include "GameState.hpp"
#include "PixelFont.hpp"
#include "Scoreboard.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

Hud::Hud(int widthi, int heighti) :
shader("./shaders/hud.vs", "./shaders/hud.fs"),
width(widthi),
height(heighti)
{
    if (!shader.isValid())
    {
        return;
    }

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

    projectionLocation =
        glGetUniformLocation(shader.ID, "projection");

    hudColorLocation =
        glGetUniformLocation(shader.ID, "hudColor");
}

Hud::~Hud()
{
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
    }
}

void Hud::beginRender()
{
    shader.use();
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    const glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f
    );

    glUniformMatrix4fv(
        projectionLocation,
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );
}

void Hud::endRender()
{
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
        x,                  y,
        x + rectangleWidth, y,
        x + rectangleWidth, y + rectangleHeight,

        x,                  y,
        x + rectangleWidth, y + rectangleHeight,
        x,                  y + rectangleHeight
    };

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glUniform4f(
        hudColorLocation,
        color.x,
        color.y,
        color.z,
        alpha
    );

    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    const glm::vec3 yellow(1.0f, 0.85f, 0.0f);
    const glm::vec3 blue(0.0f, 0.4f, 1.0f);
    const glm::vec3 darkBlue(0.01f, 0.06f, 0.18f);
    const glm::vec3 panelColor(0.008f, 0.018f, 0.055f);
    const glm::vec3 lightGray(0.86f, 0.88f, 0.94f);
    const glm::vec3 mutedGray(0.42f, 0.46f, 0.55f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(
        50.0f,
        25.0f,
        700.0f,
        550.0f,
        blue
    );

    drawRectangle(
        54.0f,
        29.0f,
        692.0f,
        542.0f,
        panelColor
    );

    drawRectangle(
        88.0f,
        101.0f,
        115.0f,
        3.0f,
        blue
    );

    drawRectangle(
        597.0f,
        101.0f,
        115.0f,
        3.0f,
        blue
    );

    const std::string title{"PACBOY"};
    constexpr float titleScale{9.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 4.0f,
        69.0f,
        titleScale,
        darkBlue
    );

    renderText(
        title,
        titleX,
        65.0f,
        titleScale,
        yellow
    );

    const std::string subtitle{"ARCADE EDITION"};
    constexpr float subtitleScale{2.5f};

    renderText(
        subtitle,
        centeredX(subtitle, subtitleScale),
        145.0f,
        subtitleScale,
        lightGray
    );

    drawRectangle(
        145.0f,
        195.0f,
        510.0f,
        235.0f,
        darkBlue
    );

    drawRectangle(
        149.0f,
        199.0f,
        502.0f,
        227.0f,
        glm::vec3(0.012f, 0.03f, 0.09f)
    );

    const std::string options[]{
        "START GAME",
        "SCOREBOARD"
    };

    constexpr float optionScale{4.5f};
    constexpr float firstOptionY{235.0f};
    constexpr float optionSpacing{82.0f};

    for (int option = 0; option < 2; option++)
    {
        const float optionY =
            firstOptionY + option * optionSpacing;

        const bool isSelected = option == selected;
        const std::string& optionText = options[option];
        const float optionX = centeredX(optionText, optionScale);

        if (isSelected)
        {
            drawRectangle(
                172.0f,
                optionY - 13.0f,
                456.0f,
                58.0f,
                darkBlue
            );

            drawRectangle(
                172.0f,
                optionY - 13.0f,
                456.0f,
                2.0f,
                yellow
            );

            drawRectangle(
                172.0f,
                optionY + 43.0f,
                456.0f,
                2.0f,
                yellow
            );
        }

        renderText(
            optionText,
            optionX,
            optionY,
            optionScale,
            isSelected ? yellow : lightGray
        );

        if (isSelected)
        {
            renderText(
                ">",
                optionX - 42.0f,
                optionY,
                optionScale,
                yellow
            );

            renderText(
                "<",
                optionX + textWidth(optionText, optionScale) + 15.0f,
                optionY,
                optionScale,
                yellow
            );
        }
    }

    const std::string navigationHint{"USE ARROWS TO SELECT"};
    const std::string confirmHint{"ENTER TO CONFIRM"};
    const std::string exitHint{"ESC TO EXIT"};
    constexpr float hintScale{2.0f};

    renderText(
        navigationHint,
        centeredX(navigationHint, hintScale),
        460.0f,
        hintScale,
        lightGray
    );

    renderText(
        confirmHint,
        centeredX(confirmHint, hintScale),
        490.0f,
        hintScale,
        blue
    );

    renderText(
        exitHint,
        centeredX(exitHint, hintScale),
        535.0f,
        hintScale,
        mutedGray
    );
}

void Hud::renderReady()
{
    const glm::vec3 yellow(1.0f, 0.85f, 0.0f);
    const glm::vec3 blue(0.0f, 0.4f, 1.0f);
    const glm::vec3 darkBlue(0.01f, 0.04f, 0.13f);
    const glm::vec3 lightGray(0.86f, 0.88f, 0.94f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.35f
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(
        135.0f,
        205.0f,
        530.0f,
        190.0f,
        yellow,
        0.95f
    );

    drawRectangle(
        139.0f,
        209.0f,
        522.0f,
        182.0f,
        darkBlue,
        0.92f
    );

    const std::string title{"READY!"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 4.0f,
        239.0f,
        titleScale,
        glm::vec3(0.2f, 0.12f, 0.0f)
    );

    renderText(
        title,
        titleX,
        235.0f,
        titleScale,
        yellow
    );

    drawRectangle(
        190.0f,
        315.0f,
        420.0f,
        2.0f,
        blue
    );

    const std::string hint{"CHOOSE YOUR DIRECTION"};
    constexpr float hintScale{2.5f};

    renderText(
        hint,
        centeredX(hint, hintScale),
        340.0f,
        hintScale,
        lightGray
    );
}

void Hud::renderLevelComplete(const GameState& state)
{
    const glm::vec3 yellow(1.0f, 0.85f, 0.0f);
    const glm::vec3 blue(0.0f, 0.4f, 1.0f);
    const glm::vec3 darkBlue(0.01f, 0.04f, 0.13f);
    const glm::vec3 lightGray(0.86f, 0.88f, 0.94f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f),
        0.78f
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(80.0f, 95.0f, 640.0f, 410.0f, blue);
    drawRectangle(84.0f, 99.0f, 632.0f, 402.0f, darkBlue, 0.96f);

    const std::string title{"LEVEL COMPLETE"};
    constexpr float titleScale{6.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 4.0f,
        139.0f,
        titleScale,
        glm::vec3(0.18f, 0.12f, 0.0f)
    );
    renderText(title, titleX, 135.0f, titleScale, yellow);

    drawRectangle(160.0f, 220.0f, 480.0f, 3.0f, blue);

    const std::string level{
        "LEVEL " + std::to_string(state.getLevel())
    };
    constexpr float informationScale{3.0f};

    renderText(
        level,
        centeredX(level, informationScale),
        260.0f,
        informationScale,
        lightGray
    );

    const std::string score{
        "SCORE " + std::to_string(state.getScore())
    };

    renderText(
        score,
        centeredX(score, informationScale),
        310.0f,
        informationScale,
        yellow
    );

    const std::string prompt{"ENTER TO CONTINUE"};
    constexpr float promptScale{2.5f};

    drawRectangle(190.0f, 398.0f, 420.0f, 52.0f, blue);
    drawRectangle(194.0f, 402.0f, 412.0f, 44.0f, darkBlue);
    renderText(
        prompt,
        centeredX(prompt, promptScale),
        414.0f,
        promptScale,
        lightGray
    );
}

void Hud::renderPause(int selected)
{
    const glm::vec3 yellow(1.0f, 0.85f, 0.0f);
    const glm::vec3 blue(0.0f, 0.4f, 1.0f);
    const glm::vec3 darkBlue(0.01f, 0.05f, 0.16f);
    const glm::vec3 panelColor(0.008f, 0.018f, 0.055f);
    const glm::vec3 lightGray(0.86f, 0.88f, 0.94f);
    const glm::vec3 mutedGray(0.42f, 0.46f, 0.55f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.74f
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(
        110.0f,
        55.0f,
        580.0f,
        490.0f,
        blue
    );

    drawRectangle(
        114.0f,
        59.0f,
        572.0f,
        482.0f,
        panelColor
    );

    const std::string title{"PAUSED"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 4.0f,
        94.0f,
        titleScale,
        darkBlue
    );

    renderText(
        title,
        titleX,
        90.0f,
        titleScale,
        yellow
    );

    const std::string subtitle{"GAME PAUSED"};
    constexpr float subtitleScale{2.0f};

    renderText(
        subtitle,
        centeredX(subtitle, subtitleScale),
        160.0f,
        subtitleScale,
        lightGray
    );

    drawRectangle(
        155.0f,
        205.0f,
        490.0f,
        215.0f,
        darkBlue
    );

    drawRectangle(
        159.0f,
        209.0f,
        482.0f,
        207.0f,
        glm::vec3(0.012f, 0.03f, 0.09f)
    );

    const std::string options[]{
        "RESUME",
        "MAIN MENU"
    };

    constexpr float optionScale{4.5f};
    constexpr float firstOptionY{240.0f};
    constexpr float optionSpacing{82.0f};

    for (int option = 0; option < 2; option++)
    {
        const float optionY =
            firstOptionY + option * optionSpacing;

        const bool isSelected = option == selected;
        const std::string& optionText = options[option];
        const float optionX = centeredX(optionText, optionScale);

        if (isSelected)
        {
            drawRectangle(
                182.0f,
                optionY - 13.0f,
                436.0f,
                58.0f,
                darkBlue
            );

            drawRectangle(
                182.0f,
                optionY - 13.0f,
                436.0f,
                2.0f,
                yellow
            );

            drawRectangle(
                182.0f,
                optionY + 43.0f,
                436.0f,
                2.0f,
                yellow
            );
        }

        renderText(
            optionText,
            optionX,
            optionY,
            optionScale,
            isSelected ? yellow : lightGray
        );

        if (isSelected)
        {
            renderText(
                ">",
                optionX - 42.0f,
                optionY,
                optionScale,
                yellow
            );

            renderText(
                "<",
                optionX + textWidth(optionText, optionScale) + 15.0f,
                optionY,
                optionScale,
                yellow
            );
        }
    }

    const std::string navigationHint{"USE ARROWS AND ENTER"};
    const std::string resumeHint{"P TO RESUME"};
    const std::string exitHint{"ESC TO EXIT"};
    constexpr float informationScale{2.0f};

    renderText(
        navigationHint,
        centeredX(navigationHint, informationScale),
        447.0f,
        informationScale,
        lightGray
    );

    renderText(
        resumeHint,
        centeredX(resumeHint, informationScale),
        477.0f,
        informationScale,
        blue
    );

    renderText(
        exitHint,
        centeredX(exitHint, informationScale),
        510.0f,
        informationScale,
        mutedGray
    );
}

void Hud::renderScoreboard(const Scoreboard& scoreboard)
{
    const glm::vec3 yellow(1.0f, 1.0f, 0.0f);
    const glm::vec3 blue(0.0f, 0.35f, 1.0f);
    const glm::vec3 panelColor(0.02f, 0.04f, 0.12f);
    const glm::vec3 lightGray(0.88f, 0.88f, 0.88f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    const std::string title{"HIGH SCORES"};
    constexpr float titleScale{7.0f};

    renderText(
        title,
        centeredX(title, titleScale),
        42.0f,
        titleScale,
        yellow
    );

    drawRectangle(
        105.0f,
        122.0f,
        590.0f,
        356.0f,
        blue
    );

    drawRectangle(
        108.0f,
        125.0f,
        584.0f,
        350.0f,
        panelColor
    );

    constexpr float rowScale{3.0f};

    renderText(
        "RANK",
        192.0f - textWidth("RANK", rowScale),
        140.0f,
        rowScale,
        blue
    );

    renderText(
        "PLAYER",
        230.0f,
        140.0f,
        rowScale,
        blue
    );

    renderText(
        "SCORE",
        660.0f - textWidth("SCORE", rowScale),
        140.0f,
        rowScale,
        blue
    );

    drawRectangle(
        125.0f,
        169.0f,
        550.0f,
        3.0f,
        blue
    );

    const auto& entries = scoreboard.getScoreEntries();

    if (entries.empty())
    {
        const std::string emptyMessage{"NO SCORES YET"};
        constexpr float emptyScale{4.0f};

        renderText(
            emptyMessage,
            centeredX(emptyMessage, emptyScale),
            270.0f,
            emptyScale,
            lightGray
        );
    }
    else
    {
        for (std::size_t i = 0; i < entries.size() && i < 10; i++)
        {
            const float rowY =
                183.0f + static_cast<float>(i) * 28.0f;

            if (i % 2 == 0)
            {
                drawRectangle(
                    120.0f,
                    rowY - 3.0f,
                    560.0f,
                    27.0f,
                    glm::vec3(0.035f, 0.065f, 0.17f)
                );
            }

            glm::vec3 rowColor = lightGray;

            if (i == 0)
            {
                rowColor = yellow;
            }
            else if (i == 1)
            {
                rowColor = glm::vec3(0.65f, 0.9f, 1.0f);
            }
            else if (i == 2)
            {
                rowColor = glm::vec3(1.0f, 0.55f, 0.2f);
            }

            const std::string rank = std::to_string(i + 1);
            const std::string playerName = entries[i].name.substr(0, 12);
            const std::string score = std::to_string(entries[i].score);

            renderText(
                rank,
                180.0f - textWidth(rank, rowScale),
                rowY,
                rowScale,
                rowColor
            );

            renderText(
                playerName,
                230.0f,
                rowY,
                rowScale,
                rowColor
            );

            renderText(
                score,
                660.0f - textWidth(score, rowScale),
                rowY,
                rowScale,
                rowColor
            );
        }
    }

    constexpr float buttonX{210.0f};
    constexpr float buttonY{512.0f};
    constexpr float buttonWidth{380.0f};
    constexpr float buttonHeight{48.0f};
    constexpr float borderThickness{2.0f};

    drawRectangle(
        buttonX,
        buttonY,
        buttonWidth,
        buttonHeight,
        glm::vec3(0.02f, 0.08f, 0.22f)
    );

    drawRectangle(
        buttonX,
        buttonY,
        buttonWidth,
        borderThickness,
        blue
    );

    drawRectangle(
        buttonX,
        buttonY + buttonHeight - borderThickness,
        buttonWidth,
        borderThickness,
        blue
    );

    drawRectangle(
        buttonX,
        buttonY,
        borderThickness,
        buttonHeight,
        blue
    );

    drawRectangle(
        buttonX + buttonWidth - borderThickness,
        buttonY,
        borderThickness,
        buttonHeight,
        blue
    );

    const std::string returnButton{"> RETURN TO MENU <"};
    constexpr float buttonScale{3.0f};

    renderText(
        returnButton,
        centeredX(returnButton, buttonScale),
        526.0f,
        buttonScale,
        yellow
    );
}

void Hud::renderGameOver(const GameState& state)
{
    const glm::vec3 red(1.0f, 0.08f, 0.12f);
    const glm::vec3 darkRed(0.28f, 0.01f, 0.025f);
    const glm::vec3 panelColor(0.055f, 0.008f, 0.015f);
    const glm::vec3 yellow(1.0f, 0.85f, 0.0f);
    const glm::vec3 lightGray(0.82f, 0.82f, 0.82f);

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.88f
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(
        90.0f,
        65.0f,
        620.0f,
        470.0f,
        red
    );

    drawRectangle(
        94.0f,
        69.0f,
        612.0f,
        462.0f,
        panelColor
    );

    const std::string title{"GAME OVER"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 4.0f,
        109.0f,
        titleScale,
        darkRed
    );

    renderText(
        title,
        titleX,
        105.0f,
        titleScale,
        red
    );

    drawRectangle(
        135.0f,
        188.0f,
        530.0f,
        3.0f,
        darkRed
    );

    const std::string scoreLabel{"FINAL SCORE"};
    constexpr float labelScale{3.0f};

    renderText(
        scoreLabel,
        centeredX(scoreLabel, labelScale),
        218.0f,
        labelScale,
        lightGray
    );

    const std::string score = std::to_string(state.getScore());
    constexpr float scoreScale{6.0f};

    renderText(
        score,
        centeredX(score, scoreScale),
        258.0f,
        scoreScale,
        yellow
    );

    drawRectangle(
        230.0f,
        325.0f,
        340.0f,
        2.0f,
        darkRed
    );

    const std::string levelLabel{"LEVEL REACHED"};

    renderText(
        levelLabel,
        centeredX(levelLabel, labelScale),
        352.0f,
        labelScale,
        lightGray
    );

    const std::string level = std::to_string(state.getLevel());
    constexpr float levelScale{5.0f};

    renderText(
        level,
        centeredX(level, levelScale),
        390.0f,
        levelScale,
        red
    );

    constexpr float promptX{190.0f};
    constexpr float promptY{463.0f};
    constexpr float promptWidth{420.0f};
    constexpr float promptHeight{48.0f};
    constexpr float borderThickness{2.0f};

    drawRectangle(
        promptX,
        promptY,
        promptWidth,
        promptHeight,
        darkRed
    );

    drawRectangle(
        promptX,
        promptY,
        promptWidth,
        borderThickness,
        red
    );

    drawRectangle(
        promptX,
        promptY + promptHeight - borderThickness,
        promptWidth,
        borderThickness,
        red
    );

    drawRectangle(
        promptX,
        promptY,
        borderThickness,
        promptHeight,
        red
    );

    drawRectangle(
        promptX + promptWidth - borderThickness,
        promptY,
        borderThickness,
        promptHeight,
        red
    );

    const std::string prompt{"PRESS ENTER TO CONTINUE"};
    constexpr float promptScale{2.5f};

    renderText(
        prompt,
        centeredX(prompt, promptScale),
        478.0f,
        promptScale,
        yellow
    );
}

void Hud::renderHighScore(
    const std::string& playerName,
    int score,
    bool isNewHighestScore
)
{
    const glm::vec3 gold(1.0f, 0.82f, 0.0f);
    const glm::vec3 orange(1.0f, 0.42f, 0.05f);
    const glm::vec3 blue(0.0f, 0.45f, 1.0f);
    const glm::vec3 darkGold(0.28f, 0.17f, 0.0f);
    const glm::vec3 darkBlue(0.01f, 0.08f, 0.24f);
    const glm::vec3 panelColor(0.008f, 0.018f, 0.055f);
    const glm::vec3 lightGray(0.88f, 0.9f, 0.95f);
    const glm::vec3 mutedGray(0.42f, 0.46f, 0.55f);

    const glm::vec3 accent = isNewHighestScore ? gold : blue;
    const glm::vec3 darkAccent =
        isNewHighestScore ? darkGold : darkBlue;

    drawRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    const auto textWidth = [](const std::string& text, float scale)
    {
        return
            (
                static_cast<float>(text.size()) *
                (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
                PixelFont::GLYPH_SPACING
            ) * scale;
    };

    const auto centeredX =
        [this, &textWidth](const std::string& text, float scale)
    {
        return
            (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
    };

    drawRectangle(
        55.0f,
        25.0f,
        690.0f,
        550.0f,
        accent
    );

    drawRectangle(
        59.0f,
        29.0f,
        682.0f,
        542.0f,
        panelColor
    );

    drawRectangle(
        82.0f,
        48.0f,
        105.0f,
        3.0f,
        accent
    );

    drawRectangle(
        613.0f,
        48.0f,
        105.0f,
        3.0f,
        accent
    );

    const std::string title =
        isNewHighestScore ? "NEW HIGH SCORE" : "TOP 10 SCORE";

    constexpr float titleScale{6.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(
        title,
        titleX + 3.0f,
        58.0f,
        titleScale,
        darkAccent
    );

    renderText(
        title,
        titleX,
        55.0f,
        titleScale,
        accent
    );

    const std::string subtitle =
        isNewHighestScore
            ? "YOU SET A NEW RECORD"
            : "YOU MADE THE TOP 10";

    constexpr float subtitleScale{2.5f};

    renderText(
        subtitle,
        centeredX(subtitle, subtitleScale),
        112.0f,
        subtitleScale,
        isNewHighestScore ? orange : lightGray
    );

    drawRectangle(
        270.0f,
        145.0f,
        260.0f,
        100.0f,
        darkAccent
    );

    drawRectangle(
        273.0f,
        148.0f,
        254.0f,
        94.0f,
        glm::vec3(0.015f, 0.03f, 0.085f)
    );

    const std::string scoreLabel{"SCORE"};
    constexpr float scoreLabelScale{2.5f};

    renderText(
        scoreLabel,
        centeredX(scoreLabel, scoreLabelScale),
        158.0f,
        scoreLabelScale,
        lightGray
    );

    const std::string scoreText = std::to_string(score);
    constexpr float scoreScale{5.5f};

    renderText(
        scoreText,
        centeredX(scoreText, scoreScale),
        190.0f,
        scoreScale,
        gold
    );

    drawRectangle(
        125.0f,
        260.0f,
        550.0f,
        2.0f,
        darkAccent
    );

    const std::string namePrompt{"ENTER YOUR NAME"};
    constexpr float namePromptScale{3.0f};

    renderText(
        namePrompt,
        centeredX(namePrompt, namePromptScale),
        276.0f,
        namePromptScale,
        lightGray
    );

    constexpr float inputX{155.0f};
    constexpr float inputY{307.0f};
    constexpr float inputWidth{490.0f};
    constexpr float inputHeight{130.0f};

    drawRectangle(
        inputX,
        inputY,
        inputWidth,
        inputHeight,
        accent
    );

    drawRectangle(
        inputX + 3.0f,
        inputY + 3.0f,
        inputWidth - 6.0f,
        inputHeight - 6.0f,
        glm::vec3(0.01f, 0.025f, 0.075f)
    );

    constexpr std::size_t maxNameLength{10};
    constexpr float nameScale{5.0f};
    constexpr float slotStep{
        (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) * nameScale
    };
    constexpr float slotWidth{PixelFont::GLYPH_WIDTH * nameScale};
    constexpr float slotsWidth{
        (maxNameLength - 1) * slotStep + slotWidth
    };

    const float slotsX =
        (static_cast<float>(width) - slotsWidth) / 2.0f;

    const std::string visibleName =
        playerName.substr(0, maxNameLength);

    for (std::size_t i = 0; i < maxNameLength; i++)
    {
        const float slotX =
            slotsX + static_cast<float>(i) * slotStep;

        drawRectangle(
            slotX,
            392.0f,
            slotWidth,
            2.0f,
            mutedGray
        );

        if (i < visibleName.size())
        {
            renderText(
                std::string(1, visibleName[i]),
                slotX,
                342.0f,
                nameScale,
                lightGray
            );
        }
    }

    if (visibleName.size() < maxNameLength)
    {
        const float activeSlotX =
            slotsX +
            static_cast<float>(visibleName.size()) * slotStep;

        drawRectangle(
            activeSlotX,
            389.0f,
            slotWidth,
            5.0f,
            accent
        );
    }

    const std::string characterHint{"USE LETTERS A-Z"};
    constexpr float hintScale{2.0f};

    renderText(
        characterHint,
        centeredX(characterHint, hintScale),
        455.0f,
        hintScale,
        mutedGray
    );

    constexpr float saveButtonX{205.0f};
    constexpr float saveButtonY{484.0f};
    constexpr float saveButtonWidth{390.0f};
    constexpr float saveButtonHeight{48.0f};
    constexpr float borderThickness{2.0f};

    drawRectangle(
        saveButtonX,
        saveButtonY,
        saveButtonWidth,
        saveButtonHeight,
        darkAccent
    );

    drawRectangle(
        saveButtonX,
        saveButtonY,
        saveButtonWidth,
        borderThickness,
        accent
    );

    drawRectangle(
        saveButtonX,
        saveButtonY + saveButtonHeight - borderThickness,
        saveButtonWidth,
        borderThickness,
        accent
    );

    drawRectangle(
        saveButtonX,
        saveButtonY,
        borderThickness,
        saveButtonHeight,
        accent
    );

    drawRectangle(
        saveButtonX + saveButtonWidth - borderThickness,
        saveButtonY,
        borderThickness,
        saveButtonHeight,
        accent
    );

    const std::string savePrompt{"> PRESS ENTER TO SAVE <"};
    constexpr float savePromptScale{2.5f};

    renderText(
        savePrompt,
        centeredX(savePrompt, savePromptScale),
        499.0f,
        savePromptScale,
        gold
    );

    const std::string deleteHint{"BACKSPACE TO DELETE"};

    renderText(
        deleteHint,
        centeredX(deleteHint, hintScale),
        548.0f,
        hintScale,
        mutedGray
    );
}
