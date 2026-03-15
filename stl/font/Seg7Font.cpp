#include "../Font.h"

static const bool DIGIT_SEGS[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}, // 9
};

// Segments as rectangles in [0,1]x[0,2] digit space.
// All horizontal bars span full width; verticals fit between them.
// No gaps: adjacent active segments share edges directly.
//   a = top horiz  b = top right  c = bot right  d = bot horiz
//   e = bot left   f = top left   g = mid horiz
std::array<Rect2D, 7> Seg7Font::makeSegs() const {
    double h = sw * 0.5;  // half-stroke used for vertical junction offsets
    return {{
        {0.0,   2.0-sw, 1.0,   2.0    },  // a
        {1.0-sw, 1.0+h, 1.0,   2.0-sw },  // b
        {1.0-sw, sw,    1.0,   1.0-h  },  // c
        {0.0,   0.0,    1.0,   sw     },  // d
        {0.0,   sw,     sw,    1.0-h  },  // e
        {0.0,   1.0+h,  sw,    2.0-sw },  // f
        {0.0,   1.0-h,  1.0,   1.0+h  },  // g
    }};
}

FontGlyph Seg7Font::build(size_t label, double scale, int maxDigits, size_t sideCount) const
{
    auto segs = makeSegs();
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, 1.0, startX, startY);

    FontGlyph g;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        for (int s = 0; s < 7; ++s) {
            if (!DIGIT_SEGS[digits[i]][s]) continue;
            const auto& r = segs[s];
            g.engRects.push_back({
                startX + dx + r[0] * scale,
                startY       + r[1] * scale,
                startX + dx + r[2] * scale,
                startY       + r[3] * scale
            });
        }
    }
    // Orientation indicator
    double hw = (maxDigits + (maxDigits - 1) * spacing) / 2.0 * scale;
    auto ind = indicatorBars(hw, scale);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}
