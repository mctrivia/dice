#pragma once
#include <vector>
#include "Common.h"

// Engraved rects for one die face label.
struct FontGlyph { std::vector<Rect2D> engRects; };

enum class FontStyle {
    Seg7,       // Classic 7-segment display
    ThinSeg7,   // 7-segment with fine strokes
    Bold,       // Extra-bold 7-segment
    Narrow,     // Condensed 7-segment
    Pixel,      // 5x7 dot-matrix pixel grid
    HeavyPixel, // 4x6 large blocky pixel grid
    SansSerif,  // Normal-looking Arabic numerals (7x9 pixel)
    Serif,      // Classic serif numerals (7x9 pixel)
    Blank,      // No engraving — smooth faces
};

// Build a glyph (digit segments + orientation indicator) for the given style.
// scale     : uniform size derived from smallest face radius.
// maxDigits : digit count of the largest label on the die.
FontGlyph buildGlyph(FontStyle style, size_t label, double scale, int maxDigits);
