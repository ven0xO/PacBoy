#pragma once

#include <array>
#include <string>
#include <unordered_map>

namespace PixelFont
{
    inline constexpr int GLYPH_WIDTH = 5;
    inline constexpr int GLYPH_HEIGHT = 7;
    inline constexpr int GLYPH_SPACING = 1;

    using Glyph = std::array<std::string, GLYPH_HEIGHT>;
    using Font = std::unordered_map<char, Glyph>;

    const Font& glyphs();
    // Borrowed from the static font table; null when no fallback exists.
    const Glyph* findGlyph(char character);
}
