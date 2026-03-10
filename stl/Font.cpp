#include "Font.h"
#include <algorithm>
#include <cmath>

// ============================================================
//  Shared helpers
// ============================================================

static std::vector<int> labelDigits(size_t label) {
    std::vector<int> d;
    size_t n = label;
    do { d.push_back((int)(n % 10)); n /= 10; } while (n > 0);
    std::reverse(d.begin(), d.end());
    return d;
}

// Compute layout metrics shared by all fonts.
// Returns startX (left edge of first digit), startY (bottom of digit area).
static void layoutMetrics(int nd, int maxDigits, double scale,
                           double spacing,
                           double& startX, double& startY)
{
    double totalMaxW   = maxDigits + (maxDigits - 1) * spacing;
    double totalLabelW = nd        + (nd       - 1) * spacing;
    startX = (-(totalMaxW / 2.0) + (totalMaxW - totalLabelW) / 2.0) * scale;
    startY = -0.8 * scale;
}

// ============================================================
//  7-segment  (parameterised by stroke width SW and gap SG)
// ============================================================

// Digit space: x in [0,1], y in [0,2].  Segments a-g:
//   a=top-horiz  b=top-right  c=bot-right  d=bot-horiz
//   e=bot-left   f=top-left   g=mid-horiz
static std::array<Rect2D,7> makeSegs(double sw, double sg) {
    return {{
        {sw,     2.0-sw,           1.0-sw, 2.0          },  // a
        {1.0-sw, 1.0+sw/2.0+sg,   1.0,    2.0-sw-sg    },  // b
        {1.0-sw, sw+sg,            1.0,    1.0-sw/2.0-sg},  // c
        {sw,     0.0,              1.0-sw, sw           },  // d
        {0.0,    sw+sg,            sw,     1.0-sw/2.0-sg},  // e
        {0.0,    1.0+sw/2.0+sg,   sw,     2.0-sw-sg    },  // f
        {sw,     1.0-sw/2.0,      1.0-sw, 1.0+sw/2.0   },  // g
    }};
}

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

static std::vector<Rect2D> buildSeg7Rects(size_t label, double scale, int maxDigits,
                                           double sw, double sg)
{
    auto segs = makeSegs(sw, sg);
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    const double spacing = 0.20;
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, startX, startY);

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        for (int s = 0; s < 7; ++s) {
            if (!DIGIT_SEGS[digits[i]][s]) continue;
            const auto& r = segs[s];
            rects.push_back({
                startX + dx + r[0] * scale,
                startY       + r[1] * scale,
                startX + dx + r[2] * scale,
                startY       + r[3] * scale
            });
        }
    }
    return rects;
}

// Three stacked bars forming a downward-pointing arrow ▼.
static std::vector<Rect2D> makeIndicatorBars(double scale, int maxDigits) {
    const double spacing = 0.20;
    double totalMaxW = maxDigits + (maxDigits - 1) * spacing;
    double hw    = totalMaxW / 2.0 * scale;
    double barH  = 0.085 * scale;
    double yBase = -1.12 * scale;
    return {
        { -hw*0.72, yBase + 2*barH, hw*0.72, yBase + 3*barH },  // top  (widest)
        { -hw*0.46, yBase +   barH, hw*0.46, yBase + 2*barH },  // mid
        { -hw*0.20, yBase,          hw*0.20, yBase +   barH },  // tip  (narrowest)
    };
}

static FontGlyph fontSeg7Impl(size_t label, double scale, int maxDigits,
                               double sw, double sg)
{
    FontGlyph g;
    g.engRects = buildSeg7Rects(label, scale, maxDigits, sw, sg);
    auto ind = makeIndicatorBars(scale, maxDigits);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}

// ============================================================
//  5x7 Dot-matrix pixel font
// ============================================================

// Row 0 = top of digit, row 6 = bottom.  Col 0 = left.
// Digit space [0,1]x[0,2], 5 cols x 7 rows.
static const bool PIXEL_DIGITS[10][7][5] = {
    // 0
    {{ 0,1,1,1,0 },
     { 1,0,0,0,1 },
     { 1,0,0,1,1 },
     { 1,0,1,0,1 },
     { 1,1,0,0,1 },
     { 1,0,0,0,1 },
     { 0,1,1,1,0 }},
    // 1
    {{ 0,0,1,0,0 },
     { 0,1,1,0,0 },
     { 0,0,1,0,0 },
     { 0,0,1,0,0 },
     { 0,0,1,0,0 },
     { 0,0,1,0,0 },
     { 0,1,1,1,0 }},
    // 2
    {{ 0,1,1,1,0 },
     { 1,0,0,0,1 },
     { 0,0,0,0,1 },
     { 0,0,1,1,0 },
     { 0,1,0,0,0 },
     { 1,0,0,0,0 },
     { 1,1,1,1,1 }},
    // 3
    {{ 1,1,1,1,0 },
     { 0,0,0,0,1 },
     { 0,0,0,0,1 },
     { 0,1,1,1,0 },
     { 0,0,0,0,1 },
     { 0,0,0,0,1 },
     { 1,1,1,1,0 }},
    // 4
    {{ 0,0,0,1,0 },
     { 0,0,1,1,0 },
     { 0,1,0,1,0 },
     { 1,0,0,1,0 },
     { 1,1,1,1,1 },
     { 0,0,0,1,0 },
     { 0,0,0,1,0 }},
    // 5
    {{ 1,1,1,1,1 },
     { 1,0,0,0,0 },
     { 1,0,0,0,0 },
     { 1,1,1,1,0 },
     { 0,0,0,0,1 },
     { 0,0,0,0,1 },
     { 1,1,1,1,0 }},
    // 6
    {{ 0,0,1,1,0 },
     { 0,1,0,0,0 },
     { 1,0,0,0,0 },
     { 1,1,1,1,0 },
     { 1,0,0,0,1 },
     { 1,0,0,0,1 },
     { 0,1,1,1,0 }},
    // 7
    {{ 1,1,1,1,1 },
     { 0,0,0,0,1 },
     { 0,0,0,1,0 },
     { 0,0,1,0,0 },
     { 0,1,0,0,0 },
     { 0,1,0,0,0 },
     { 0,1,0,0,0 }},
    // 8
    {{ 0,1,1,1,0 },
     { 1,0,0,0,1 },
     { 1,0,0,0,1 },
     { 0,1,1,1,0 },
     { 1,0,0,0,1 },
     { 1,0,0,0,1 },
     { 0,1,1,1,0 }},
    // 9
    {{ 0,1,1,1,0 },
     { 1,0,0,0,1 },
     { 1,0,0,0,1 },
     { 0,1,1,1,1 },
     { 0,0,0,0,1 },
     { 0,0,0,1,0 },
     { 0,1,1,0,0 }},
};

static std::vector<Rect2D> buildPixelRects(size_t label, double scale, int maxDigits)
{
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    const double spacing  = 0.20;
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, startX, startY);

    // Each digit occupies [0,1]x[0,2] in digit-local coords.
    // 5 cols x 7 rows of pixels; 80% fill, 20% gap per cell.
    const double CW = 1.0 / 5.0;            // cell width in digit units
    const double CH = 2.0 / 7.0;            // cell height
    const double PW = CW * 0.80;            // pixel width
    const double PH = CH * 0.80;            // pixel height
    const double GU = (CW - PW) / 2.0;     // horizontal gap margin
    const double GV = (CH - PH) / 2.0;     // vertical gap margin

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        const auto& pat = PIXEL_DIGITS[digits[i]];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (!pat[row][col]) continue;
                double x0 = col * CW + GU;
                double y0 = (6 - row) * CH + GV;   // row 0=top → high y
                rects.push_back({
                    startX + dx + x0 * scale,
                    startY       + y0 * scale,
                    startX + dx + (x0 + PW) * scale,
                    startY       + (y0 + PH) * scale
                });
            }
        }
    }
    return rects;
}

// Pixel-art downward-arrow indicator: wide row → medium row → single dot.
// Built from individual pixel squares to match the dot-matrix style.
static std::vector<Rect2D> makeIndicatorPixel(double scale, int maxDigits)
{
    const double spacing = 0.20;
    double totalMaxW = maxDigits + (maxDigits - 1) * spacing;
    double hw     = totalMaxW / 2.0 * scale;  // half-width of indicator area
    double yBase  = -1.12 * scale;
    double barH   = 0.085 * scale;
    double gap    = 0.015 * scale;            // gap between pixel columns

    // 5 evenly-spaced pixel columns spanning [-hw*0.8, hw*0.8].
    double totalW = hw * 1.6;
    double slotW  = totalW / 5.0;
    double pixW   = slotW - gap;

    // Row pattern (top to bottom): all 5 lit, middle 3, centre 1.
    static const int COL_COUNTS[3] = { 5, 3, 1 };
    static const int COL_OFFSETS[3] = { 0, 1, 2 };  // first lit col in each row

    std::vector<Rect2D> rects;
    for (int row = 0; row < 3; ++row) {
        double y0 = yBase + (2 - row) * barH;
        double y1 = y0 + barH - gap;
        int cnt    = COL_COUNTS[row];
        int first  = COL_OFFSETS[row];
        for (int c = first; c < first + cnt; ++c) {
            double x0 = -hw * 0.8 + c * slotW;
            rects.push_back({ x0, y0, x0 + pixW, y1 });
        }
    }
    return rects;
}

static FontGlyph fontPixelImpl(size_t label, double scale, int maxDigits)
{
    FontGlyph g;
    g.engRects = buildPixelRects(label, scale, maxDigits);
    auto ind = makeIndicatorPixel(scale, maxDigits);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}

// ============================================================
//  Public dispatch
// ============================================================

FontGlyph buildGlyph(FontStyle style, size_t label, double scale, int maxDigits)
{
    switch (style) {
        case FontStyle::ThinSeg7:
            return fontSeg7Impl(label, scale, maxDigits, 0.12, 0.02);
        case FontStyle::Pixel:
            return fontPixelImpl(label, scale, maxDigits);
        case FontStyle::Seg7:
        default:
            return fontSeg7Impl(label, scale, maxDigits, 0.20, 0.03);
    }
}
