#include "Hud.hpp"
#include "GameState.hpp"
#include "PixelFont.hpp"
#include "Scoreboard.hpp"
#include "../external/GLAD/include/glad/glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

namespace
{
    namespace HudPalette
    {
        const glm::vec3 background{0.002f, 0.006f, 0.02f};
        const glm::vec3 panel{0.008f, 0.018f, 0.055f};
        const glm::vec3 inset{0.012f, 0.03f, 0.09f};
        const glm::vec3 darkBlue{0.01f, 0.06f, 0.18f};
        const glm::vec3 electricBlue{0.0f, 0.42f, 1.0f};
        const glm::vec3 yellow{1.0f, 0.85f, 0.0f};
        const glm::vec3 lightGray{0.86f, 0.88f, 0.94f};
        const glm::vec3 mutedGray{0.42f, 0.46f, 0.55f};
        const glm::vec3 red{1.0f, 0.08f, 0.12f};
        const glm::vec3 darkRed{0.28f, 0.01f, 0.025f};
        const glm::vec3 gold{1.0f, 0.82f, 0.0f};
        const glm::vec3 darkGold{0.28f, 0.17f, 0.0f};
        const glm::vec3 goldShadow{0.2f, 0.12f, 0.0f};
        const glm::vec3 orange{1.0f, 0.42f, 0.05f};
        const glm::vec3 cyan{0.65f, 0.9f, 1.0f};
        const glm::vec3 bronze{1.0f, 0.55f, 0.2f};
        const glm::vec3 rowHighlight{0.035f, 0.065f, 0.17f};
    } // namespace HudPalette

    constexpr float frameThickness{4.0f};
    constexpr float buttonBorderThickness{2.0f};
} // namespace

Hud::Hud(int widthi, int heighti)
    : shader("./shaders/hud.vs", "./shaders/hud.fs"), width(widthi), height(heighti)
{
    if (!shader.isValid())
    {
        return;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    projectionLocation = glGetUniformLocation(shader.ID, "projection");

    hudColorLocation = glGetUniformLocation(shader.ID, "hudColor");
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

    const glm::mat4 projection =
        glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
}

void Hud::endRender()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Hud::render(const GameState& state)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), 38.0f, HudPalette::background);

    renderText("SCORE " + std::to_string(state.getScore()), 12.0f, 8.0f, 3.0f, HudPalette::yellow);

    renderText("LIVES " + std::to_string(state.getLives()), 300.0f, 8.0f, 3.0f, HudPalette::yellow);

    renderText("LEVEL " + std::to_string(state.getLevel()), 600.0f, 8.0f, 3.0f, HudPalette::yellow);
}

void Hud::drawRectangle(float x, float y, float rectangleWidth, float rectangleHeight,
                        glm::vec3 color, float alpha)
{
    float vertices[] = {x,
                        y,
                        x + rectangleWidth,
                        y,
                        x + rectangleWidth,
                        y + rectangleHeight,

                        x,
                        y,
                        x + rectangleWidth,
                        y + rectangleHeight,
                        x,
                        y + rectangleHeight};

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glUniform4f(hudColorLocation, color.x, color.y, color.z, alpha);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Hud::drawPanel(float x, float y, float panelWidth, float panelHeight,
                    const glm::vec3& borderColor, const glm::vec3& fillColor, float borderThickness,
                    float fillAlpha)
{
    drawRectangle(x, y, panelWidth, panelHeight, borderColor);
    drawRectangle(x + borderThickness, y + borderThickness, panelWidth - 2.0f * borderThickness,
                  panelHeight - 2.0f * borderThickness, fillColor, fillAlpha);
}

void Hud::drawSelection(float x, float y, float selectionWidth, float selectionHeight,
                        const glm::vec3& accentColor)
{
    drawRectangle(x, y, selectionWidth, selectionHeight, HudPalette::darkBlue);
    drawRectangle(x, y, selectionWidth, buttonBorderThickness, accentColor);
    drawRectangle(x, y + selectionHeight - buttonBorderThickness, selectionWidth,
                  buttonBorderThickness, accentColor);
}

float Hud::textWidth(const std::string& text, float scale) const
{
    return (static_cast<float>(text.size()) * (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) -
            PixelFont::GLYPH_SPACING) *
           scale;
}

float Hud::centeredX(const std::string& text, float scale) const
{
    return (static_cast<float>(width) - textWidth(text, scale)) / 2.0f;
}

void Hud::renderText(const std::string& text, float x, float y, float scale, glm::vec3 textColor)
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
                        drawRectangle(cursorX + static_cast<float>(column) * scale,
                                      y + static_cast<float>(row) * scale, scale, scale, textColor);
                    }
                }
            }
        }

        cursorX += (PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) * scale;
    }
}

void Hud::renderMainMenu(int selected)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background);

    drawPanel(50.0f, 25.0f, 700.0f, 550.0f, HudPalette::electricBlue, HudPalette::panel);

    drawRectangle(88.0f, 101.0f, 115.0f, 3.0f, HudPalette::electricBlue);

    drawRectangle(597.0f, 101.0f, 115.0f, 3.0f, HudPalette::electricBlue);

    const std::string title{"PACBOY"};
    constexpr float titleScale{9.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 4.0f, 69.0f, titleScale, HudPalette::darkBlue);

    renderText(title, titleX, 65.0f, titleScale, HudPalette::yellow);

    const std::string subtitle{"ARCADE EDITION"};
    constexpr float subtitleScale{2.5f};

    renderText(subtitle, centeredX(subtitle, subtitleScale), 145.0f, subtitleScale,
               HudPalette::lightGray);

    drawPanel(145.0f, 195.0f, 510.0f, 235.0f, HudPalette::darkBlue, HudPalette::inset);

    const std::string options[]{"START GAME", "SCOREBOARD"};

    constexpr float optionScale{4.5f};
    constexpr float firstOptionY{235.0f};
    constexpr float optionSpacing{82.0f};

    for (int option = 0; option < 2; option++)
    {
        const float optionY = firstOptionY + static_cast<float>(option) * optionSpacing;

        const bool isSelected = option == selected;
        const std::string& optionText = options[option];
        const float optionX = centeredX(optionText, optionScale);

        if (isSelected)
        {
            drawSelection(172.0f, optionY - 13.0f, 456.0f, 58.0f, HudPalette::yellow);
        }

        renderText(optionText, optionX, optionY, optionScale,
                   isSelected ? HudPalette::yellow : HudPalette::lightGray);

        if (isSelected)
        {
            renderText(">", optionX - 42.0f, optionY, optionScale, HudPalette::yellow);

            renderText("<", optionX + textWidth(optionText, optionScale) + 15.0f, optionY,
                       optionScale, HudPalette::yellow);
        }
    }

    const std::string navigationHint{"USE ARROWS TO SELECT"};
    const std::string confirmHint{"ENTER TO CONFIRM"};
    const std::string exitHint{"ESC TO EXIT"};
    constexpr float hintScale{2.0f};

    renderText(navigationHint, centeredX(navigationHint, hintScale), 460.0f, hintScale,
               HudPalette::lightGray);

    renderText(confirmHint, centeredX(confirmHint, hintScale), 490.0f, hintScale,
               HudPalette::electricBlue);

    renderText(exitHint, centeredX(exitHint, hintScale), 535.0f, hintScale, HudPalette::mutedGray);
}

void Hud::renderReady()
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background, 0.35f);

    drawPanel(135.0f, 205.0f, 530.0f, 190.0f, HudPalette::yellow, HudPalette::panel, frameThickness,
              0.92f);

    const std::string title{"READY!"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 4.0f, 239.0f, titleScale, HudPalette::goldShadow);

    renderText(title, titleX, 235.0f, titleScale, HudPalette::yellow);

    drawRectangle(190.0f, 315.0f, 420.0f, 2.0f, HudPalette::electricBlue);

    const std::string hint{"CHOOSE YOUR DIRECTION"};
    constexpr float hintScale{2.5f};

    renderText(hint, centeredX(hint, hintScale), 340.0f, hintScale, HudPalette::lightGray);
}

void Hud::renderLevelComplete(const GameState& state)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background, 0.78f);

    drawPanel(80.0f, 95.0f, 640.0f, 410.0f, HudPalette::electricBlue, HudPalette::panel,
              frameThickness, 0.96f);

    const std::string title{"LEVEL COMPLETE"};
    constexpr float titleScale{6.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 4.0f, 139.0f, titleScale, HudPalette::goldShadow);
    renderText(title, titleX, 135.0f, titleScale, HudPalette::yellow);

    drawRectangle(160.0f, 220.0f, 480.0f, 3.0f, HudPalette::electricBlue);

    const std::string level{"LEVEL " + std::to_string(state.getLevel())};
    constexpr float informationScale{3.0f};

    renderText(level, centeredX(level, informationScale), 260.0f, informationScale,
               HudPalette::lightGray);

    const std::string score{"SCORE " + std::to_string(state.getScore())};

    renderText(score, centeredX(score, informationScale), 310.0f, informationScale,
               HudPalette::yellow);

    const std::string prompt{"ENTER TO CONTINUE"};
    constexpr float promptScale{2.5f};

    drawPanel(190.0f, 398.0f, 420.0f, 52.0f, HudPalette::electricBlue, HudPalette::darkBlue);
    renderText(prompt, centeredX(prompt, promptScale), 414.0f, promptScale, HudPalette::lightGray);
}

void Hud::renderPause(int selected)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background, 0.74f);

    drawPanel(110.0f, 55.0f, 580.0f, 490.0f, HudPalette::electricBlue, HudPalette::panel);

    const std::string title{"PAUSED"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 4.0f, 94.0f, titleScale, HudPalette::darkBlue);

    renderText(title, titleX, 90.0f, titleScale, HudPalette::yellow);

    const std::string subtitle{"GAME PAUSED"};
    constexpr float subtitleScale{2.0f};

    renderText(subtitle, centeredX(subtitle, subtitleScale), 160.0f, subtitleScale,
               HudPalette::lightGray);

    drawPanel(155.0f, 205.0f, 490.0f, 215.0f, HudPalette::darkBlue, HudPalette::inset);

    const std::string options[]{"RESUME", "MAIN MENU"};

    constexpr float optionScale{4.5f};
    constexpr float firstOptionY{240.0f};
    constexpr float optionSpacing{82.0f};

    for (int option = 0; option < 2; option++)
    {
        const float optionY = firstOptionY + static_cast<float>(option) * optionSpacing;

        const bool isSelected = option == selected;
        const std::string& optionText = options[option];
        const float optionX = centeredX(optionText, optionScale);

        if (isSelected)
        {
            drawSelection(182.0f, optionY - 13.0f, 436.0f, 58.0f, HudPalette::yellow);
        }

        renderText(optionText, optionX, optionY, optionScale,
                   isSelected ? HudPalette::yellow : HudPalette::lightGray);

        if (isSelected)
        {
            renderText(">", optionX - 42.0f, optionY, optionScale, HudPalette::yellow);

            renderText("<", optionX + textWidth(optionText, optionScale) + 15.0f, optionY,
                       optionScale, HudPalette::yellow);
        }
    }

    const std::string navigationHint{"USE ARROWS AND ENTER"};
    const std::string resumeHint{"P TO RESUME"};
    const std::string exitHint{"ESC TO EXIT"};
    constexpr float informationScale{2.0f};

    renderText(navigationHint, centeredX(navigationHint, informationScale), 447.0f,
               informationScale, HudPalette::lightGray);

    renderText(resumeHint, centeredX(resumeHint, informationScale), 477.0f, informationScale,
               HudPalette::electricBlue);

    renderText(exitHint, centeredX(exitHint, informationScale), 510.0f, informationScale,
               HudPalette::mutedGray);
}

void Hud::renderScoreboard(const Scoreboard& scoreboard)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background);

    const std::string title{"HIGH SCORES"};
    constexpr float titleScale{7.0f};

    renderText(title, centeredX(title, titleScale), 42.0f, titleScale, HudPalette::yellow);

    drawPanel(105.0f, 122.0f, 590.0f, 356.0f, HudPalette::electricBlue, HudPalette::panel);

    constexpr float rowScale{3.0f};

    renderText("RANK", 192.0f - textWidth("RANK", rowScale), 140.0f, rowScale,
               HudPalette::electricBlue);

    renderText("PLAYER", 230.0f, 140.0f, rowScale, HudPalette::electricBlue);

    renderText("SCORE", 660.0f - textWidth("SCORE", rowScale), 140.0f, rowScale,
               HudPalette::electricBlue);

    drawRectangle(125.0f, 169.0f, 550.0f, 3.0f, HudPalette::electricBlue);

    const auto& entries = scoreboard.getScoreEntries();

    if (entries.empty())
    {
        const std::string emptyMessage{"NO SCORES YET"};
        constexpr float emptyScale{4.0f};

        renderText(emptyMessage, centeredX(emptyMessage, emptyScale), 270.0f, emptyScale,
                   HudPalette::lightGray);
    }
    else
    {
        for (std::size_t i = 0; i < entries.size() && i < 10; i++)
        {
            const float rowY = 183.0f + static_cast<float>(i) * 28.0f;

            if (i % 2 == 0)
            {
                drawRectangle(120.0f, rowY - 3.0f, 560.0f, 27.0f, HudPalette::rowHighlight);
            }

            glm::vec3 rowColor = HudPalette::lightGray;

            if (i == 0)
            {
                rowColor = HudPalette::gold;
            }
            else if (i == 1)
            {
                rowColor = HudPalette::cyan;
            }
            else if (i == 2)
            {
                rowColor = HudPalette::bronze;
            }

            const std::string rank = std::to_string(i + 1);
            const std::string playerName = entries[i].name.substr(0, 12);
            const std::string score = std::to_string(entries[i].score);

            renderText(rank, 180.0f - textWidth(rank, rowScale), rowY, rowScale, rowColor);

            renderText(playerName, 230.0f, rowY, rowScale, rowColor);

            renderText(score, 660.0f - textWidth(score, rowScale), rowY, rowScale, rowColor);
        }
    }

    constexpr float buttonX{210.0f};
    constexpr float buttonY{512.0f};
    constexpr float buttonWidth{380.0f};
    constexpr float buttonHeight{48.0f};
    drawPanel(buttonX, buttonY, buttonWidth, buttonHeight, HudPalette::electricBlue,
              HudPalette::darkBlue, buttonBorderThickness);

    const std::string returnButton{"> RETURN TO MENU <"};
    constexpr float buttonScale{3.0f};

    renderText(returnButton, centeredX(returnButton, buttonScale), 526.0f, buttonScale,
               HudPalette::yellow);
}

void Hud::renderGameOver(const GameState& state)
{
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background, 0.88f);

    drawPanel(90.0f, 65.0f, 620.0f, 470.0f, HudPalette::red, HudPalette::panel);

    const std::string title{"GAME OVER"};
    constexpr float titleScale{8.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 4.0f, 109.0f, titleScale, HudPalette::darkRed);

    renderText(title, titleX, 105.0f, titleScale, HudPalette::red);

    drawRectangle(135.0f, 188.0f, 530.0f, 3.0f, HudPalette::red);

    const std::string scoreLabel{"FINAL SCORE"};
    constexpr float labelScale{3.0f};

    renderText(scoreLabel, centeredX(scoreLabel, labelScale), 218.0f, labelScale,
               HudPalette::lightGray);

    const std::string score = std::to_string(state.getScore());
    constexpr float scoreScale{6.0f};

    renderText(score, centeredX(score, scoreScale), 258.0f, scoreScale, HudPalette::yellow);

    drawRectangle(230.0f, 325.0f, 340.0f, 2.0f, HudPalette::red);

    const std::string levelLabel{"LEVEL REACHED"};

    renderText(levelLabel, centeredX(levelLabel, labelScale), 352.0f, labelScale,
               HudPalette::lightGray);

    const std::string level = std::to_string(state.getLevel());
    constexpr float levelScale{5.0f};

    renderText(level, centeredX(level, levelScale), 390.0f, levelScale, HudPalette::red);

    constexpr float promptX{190.0f};
    constexpr float promptY{463.0f};
    constexpr float promptWidth{420.0f};
    constexpr float promptHeight{48.0f};
    drawPanel(promptX, promptY, promptWidth, promptHeight, HudPalette::red, HudPalette::darkRed,
              buttonBorderThickness);

    const std::string prompt{"PRESS ENTER TO CONTINUE"};
    constexpr float promptScale{2.5f};

    renderText(prompt, centeredX(prompt, promptScale), 478.0f, promptScale, HudPalette::yellow);
}

void Hud::renderHighScore(const std::string& playerName, int score, bool isNewHighestScore)
{
    const glm::vec3 accent = isNewHighestScore ? HudPalette::gold : HudPalette::electricBlue;
    const glm::vec3 darkAccent = isNewHighestScore ? HudPalette::darkGold : HudPalette::darkBlue;

    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                  HudPalette::background);

    drawPanel(55.0f, 25.0f, 690.0f, 550.0f, accent, HudPalette::panel);

    drawRectangle(82.0f, 48.0f, 105.0f, 3.0f, accent);

    drawRectangle(613.0f, 48.0f, 105.0f, 3.0f, accent);

    const std::string title = isNewHighestScore ? "NEW HIGH SCORE" : "TOP 10 SCORE";

    constexpr float titleScale{6.0f};
    const float titleX = centeredX(title, titleScale);

    renderText(title, titleX + 3.0f, 58.0f, titleScale, darkAccent);

    renderText(title, titleX, 55.0f, titleScale, accent);

    const std::string subtitle = isNewHighestScore ? "YOU SET A NEW RECORD" : "YOU MADE THE TOP 10";

    constexpr float subtitleScale{2.5f};

    renderText(subtitle, centeredX(subtitle, subtitleScale), 112.0f, subtitleScale,
               isNewHighestScore ? HudPalette::orange : HudPalette::lightGray);

    drawPanel(270.0f, 145.0f, 260.0f, 100.0f, darkAccent, HudPalette::inset, 3.0f);

    const std::string scoreLabel{"SCORE"};
    constexpr float scoreLabelScale{2.5f};

    renderText(scoreLabel, centeredX(scoreLabel, scoreLabelScale), 158.0f, scoreLabelScale,
               HudPalette::lightGray);

    const std::string scoreText = std::to_string(score);
    constexpr float scoreScale{5.5f};

    renderText(scoreText, centeredX(scoreText, scoreScale), 190.0f, scoreScale, HudPalette::gold);

    drawRectangle(125.0f, 260.0f, 550.0f, 2.0f, darkAccent);

    const std::string namePrompt{"ENTER YOUR NAME"};
    constexpr float namePromptScale{3.0f};

    renderText(namePrompt, centeredX(namePrompt, namePromptScale), 276.0f, namePromptScale,
               HudPalette::lightGray);

    constexpr float inputX{155.0f};
    constexpr float inputY{307.0f};
    constexpr float inputWidth{490.0f};
    constexpr float inputHeight{130.0f};

    drawPanel(inputX, inputY, inputWidth, inputHeight, accent, HudPalette::inset, 3.0f);

    constexpr std::size_t maxNameLength{10};
    constexpr float nameScale{5.0f};
    constexpr float slotStep{(PixelFont::GLYPH_WIDTH + PixelFont::GLYPH_SPACING) * nameScale};
    constexpr float slotWidth{PixelFont::GLYPH_WIDTH * nameScale};
    constexpr float slotsWidth{(maxNameLength - 1) * slotStep + slotWidth};

    const float slotsX = (static_cast<float>(width) - slotsWidth) / 2.0f;

    const std::string visibleName = playerName.substr(0, maxNameLength);

    for (std::size_t i = 0; i < maxNameLength; i++)
    {
        const float slotX = slotsX + static_cast<float>(i) * slotStep;

        drawRectangle(slotX, 392.0f, slotWidth, 2.0f, HudPalette::mutedGray);

        if (i < visibleName.size())
        {
            renderText(std::string(1, visibleName[i]), slotX, 342.0f, nameScale,
                       HudPalette::lightGray);
        }
    }

    if (visibleName.size() < maxNameLength)
    {
        const float activeSlotX = slotsX + static_cast<float>(visibleName.size()) * slotStep;

        drawRectangle(activeSlotX, 389.0f, slotWidth, 5.0f, accent);
    }

    const std::string characterHint{"USE LETTERS A-Z"};
    constexpr float hintScale{2.0f};

    renderText(characterHint, centeredX(characterHint, hintScale), 455.0f, hintScale,
               HudPalette::mutedGray);

    constexpr float saveButtonX{205.0f};
    constexpr float saveButtonY{484.0f};
    constexpr float saveButtonWidth{390.0f};
    constexpr float saveButtonHeight{48.0f};
    drawPanel(saveButtonX, saveButtonY, saveButtonWidth, saveButtonHeight, accent, darkAccent,
              buttonBorderThickness);

    const std::string savePrompt{"> PRESS ENTER TO SAVE <"};
    constexpr float savePromptScale{2.5f};

    renderText(savePrompt, centeredX(savePrompt, savePromptScale), 499.0f, savePromptScale,
               HudPalette::gold);

    const std::string deleteHint{"BACKSPACE TO DELETE"};

    renderText(deleteHint, centeredX(deleteHint, hintScale), 548.0f, hintScale,
               HudPalette::mutedGray);
}
