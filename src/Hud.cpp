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
