// Simple bitmap font descriptor
#pragma once

#include <cstdint>

struct BitmapFont
{
    const uint8_t* data;   // pointer to glyph data, row-major
    uint8_t width;         // glyph width in pixels
    uint8_t height;        // glyph height in pixels
    uint8_t firstChar;     // ASCII code of first glyph in data
    uint8_t charCount;     // number of glyphs stored
    uint8_t rowBytes() const { return (width + 7) / 8; }
};
