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
// digitW = width of one digit in digit-unit space (default 1.0).
static void layoutMetrics(int nd, int maxDigits, double scale,
                           double spacing, double digitW,
                           double& startX, double& startY)
{
    double totalMaxW   = maxDigits * digitW + (maxDigits - 1) * spacing;
    double totalLabelW = nd        * digitW + (nd       - 1) * spacing;
    startX = (-(totalMaxW / 2.0) + (totalMaxW - totalLabelW) / 2.0) * scale;
    startY = -0.8 * scale;
}

// Three stacked bars forming a downward-pointing arrow indicator.
// hw = half-width of the indicator in scene units (already scaled).
static std::vector<Rect2D> indicatorBars(double hw, double scale) {
    double barH  = 0.085 * scale;
    double yBase = -1.12 * scale;
    return {
        { -hw*0.72, yBase + 2*barH,  hw*0.72, yBase + 3*barH },
        { -hw*0.46, yBase +   barH,  hw*0.46, yBase + 2*barH },
        { -hw*0.20, yBase,           hw*0.20, yBase +   barH },
    };
}

// Indicator sized for digit width = 1.0 and spacing = 0.20.
static std::vector<Rect2D> makeIndicatorBars(double scale, int maxDigits) {
    double hw = (maxDigits + (maxDigits - 1) * 0.20) / 2.0 * scale;
    return indicatorBars(hw, scale);
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

// Build segments with optional x-axis compression (xRatio < 1 = narrower).
static std::vector<Rect2D> buildSeg7Rects(size_t label, double scale, int maxDigits,
                                           double sw, double sg, double xRatio = 1.0,
                                           double spacing = 0.20)
{
    auto segs = makeSegs(sw, sg);
    if (xRatio != 1.0)
        for (auto& r : segs) { r[0] *= xRatio; r[2] *= xRatio; }

    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, xRatio, startX, startY);

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (xRatio + spacing) * scale;
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

static FontGlyph fontSeg7Impl(size_t label, double scale, int maxDigits,
                               double sw, double sg,
                               double xRatio = 1.0, double spacing = 0.20)
{
    FontGlyph g;
    g.engRects = buildSeg7Rects(label, scale, maxDigits, sw, sg, xRatio, spacing);
    double hw = (maxDigits * xRatio + (maxDigits - 1) * spacing) / 2.0 * scale;
    auto ind = indicatorBars(hw, scale);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}

// ============================================================
//  5x7 Dot-matrix pixel font
// ============================================================

// Row 0 = top of digit, row 6 = bottom.  Col 0 = left.
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
    layoutMetrics(nd, maxDigits, scale, spacing, 1.0, startX, startY);

    const double CW = 1.0 / 5.0;
    const double CH = 2.0 / 7.0;
    const double PW = CW * 0.80;
    const double PH = CH * 0.80;
    const double GU = (CW - PW) / 2.0;
    const double GV = (CH - PH) / 2.0;

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        const auto& pat = PIXEL_DIGITS[digits[i]];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (!pat[row][col]) continue;
                double x0 = col * CW + GU;
                double y0 = (6 - row) * CH + GV;
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

// Pixel-art downward-arrow indicator.
static std::vector<Rect2D> makeIndicatorPixel(double scale, int maxDigits)
{
    const double spacing = 0.20;
    double totalMaxW = maxDigits + (maxDigits - 1) * spacing;
    double hw     = totalMaxW / 2.0 * scale;
    double yBase  = -1.12 * scale;
    double barH   = 0.085 * scale;
    double gap    = 0.015 * scale;

    double totalW = hw * 1.6;
    double slotW  = totalW / 5.0;
    double pixW   = slotW - gap;

    static const int COL_COUNTS[3]  = { 5, 3, 1 };
    static const int COL_OFFSETS[3] = { 0, 1, 2 };

    std::vector<Rect2D> rects;
    for (int row = 0; row < 3; ++row) {
        double y0 = yBase + (2 - row) * barH;
        double y1 = y0 + barH - gap;
        int cnt   = COL_COUNTS[row];
        int first = COL_OFFSETS[row];
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
//  4x6 Heavy Pixel font  (large blocky pixels)
// ============================================================

// Row 0 = top, Row 5 = bottom.  Col 0 = left, Col 3 = right.
static const bool HEAVY_DIGITS[10][6][4] = {
    // 0
    {{ 0,1,1,0 },
     { 1,0,0,1 },
     { 1,0,0,1 },
     { 1,0,0,1 },
     { 1,0,0,1 },
     { 0,1,1,0 }},
    // 1
    {{ 0,1,1,0 },
     { 0,0,1,0 },
     { 0,0,1,0 },
     { 0,0,1,0 },
     { 0,0,1,0 },
     { 1,1,1,1 }},
    // 2
    {{ 0,1,1,0 },
     { 1,0,0,1 },
     { 0,0,0,1 },
     { 0,1,1,0 },
     { 1,0,0,0 },
     { 1,1,1,1 }},
    // 3
    {{ 1,1,1,0 },
     { 0,0,0,1 },
     { 0,1,1,0 },
     { 0,0,0,1 },
     { 0,0,0,1 },
     { 1,1,1,0 }},
    // 4
    {{ 1,0,0,1 },
     { 1,0,0,1 },
     { 1,1,1,1 },
     { 0,0,0,1 },
     { 0,0,0,1 },
     { 0,0,0,1 }},
    // 5
    {{ 1,1,1,1 },
     { 1,0,0,0 },
     { 1,1,1,0 },
     { 0,0,0,1 },
     { 0,0,0,1 },
     { 1,1,1,0 }},
    // 6
    {{ 0,1,1,0 },
     { 1,0,0,0 },
     { 1,1,1,0 },
     { 1,0,0,1 },
     { 1,0,0,1 },
     { 0,1,1,0 }},
    // 7
    {{ 1,1,1,1 },
     { 0,0,0,1 },
     { 0,0,1,0 },
     { 0,0,1,0 },
     { 0,1,0,0 },
     { 0,1,0,0 }},
    // 8
    {{ 0,1,1,0 },
     { 1,0,0,1 },
     { 0,1,1,0 },
     { 1,0,0,1 },
     { 1,0,0,1 },
     { 0,1,1,0 }},
    // 9
    {{ 0,1,1,0 },
     { 1,0,0,1 },
     { 0,1,1,1 },
     { 0,0,0,1 },
     { 0,0,0,1 },
     { 0,1,1,0 }},
};

static std::vector<Rect2D> buildHeavyRects(size_t label, double scale, int maxDigits)
{
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    const double spacing = 0.20;
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, 1.0, startX, startY);

    const double CW = 1.0 / 4.0;
    const double CH = 2.0 / 6.0;
    const double PW = CW * 0.88;
    const double PH = CH * 0.88;
    const double GU = (CW - PW) / 2.0;
    const double GV = (CH - PH) / 2.0;

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        const auto& pat = HEAVY_DIGITS[digits[i]];
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 4; ++col) {
                if (!pat[row][col]) continue;
                double x0 = col * CW + GU;
                double y0 = (5 - row) * CH + GV;
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

static FontGlyph fontHeavyPixelImpl(size_t label, double scale, int maxDigits)
{
    FontGlyph g;
    g.engRects = buildHeavyRects(label, scale, maxDigits);
    auto ind = makeIndicatorBars(scale, maxDigits);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}

// ============================================================
//  7x9 SansSerif pixel font  (normal-looking Arabic numerals)
// ============================================================

// Row 0 = top, Row 8 = bottom.  Col 0 = left, Col 6 = right.
// Digit space [0,1]x[0,2], 7 cols x 9 rows.
static const bool SANS_DIGITS[10][9][7] = {
    // 0
    {{ 0,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,0 }},
    // 1
    {{ 0,0,1,1,0,0,0 },
     { 0,1,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,1,1,1,1,1,0 }},
    // 2
    {{ 0,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,1,1,1,0 },
     { 0,0,1,0,0,0,0 },
     { 0,1,0,0,0,0,0 },
     { 1,0,0,0,0,0,0 },
     { 1,1,1,1,1,1,1 }},
    // 3
    {{ 0,1,1,1,1,1,0 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,1,1,1,1,0 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,0 }},
    // 4
    {{ 0,0,0,0,1,0,0 },
     { 0,0,0,1,1,0,0 },
     { 0,0,1,0,1,0,0 },
     { 0,1,0,0,1,0,0 },
     { 1,0,0,0,1,0,0 },
     { 1,1,1,1,1,1,1 },
     { 0,0,0,0,1,0,0 },
     { 0,0,0,0,1,0,0 },
     { 0,0,0,0,1,0,0 }},
    // 5
    {{ 1,1,1,1,1,1,1 },
     { 1,0,0,0,0,0,0 },
     { 1,0,0,0,0,0,0 },
     { 1,0,0,0,0,0,0 },
     { 1,1,1,1,1,1,0 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 1,1,1,1,1,1,0 }},
    // 6
    {{ 0,0,1,1,1,1,0 },
     { 0,1,0,0,0,0,0 },
     { 1,0,0,0,0,0,0 },
     { 1,0,0,0,0,0,0 },
     { 1,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,0 }},
    // 7
    {{ 1,1,1,1,1,1,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,1,0 },
     { 0,0,0,0,0,1,0 },
     { 0,0,0,0,1,0,0 },
     { 0,0,0,0,1,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 }},
    // 8
    {{ 0,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,0 }},
    // 9
    {{ 0,1,1,1,1,1,0 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 1,0,0,0,0,0,1 },
     { 0,1,1,1,1,1,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,0,1 },
     { 0,0,0,0,0,1,0 },
     { 0,1,1,1,1,0,0 }},
};

static std::vector<Rect2D> buildSansRects(size_t label, double scale, int maxDigits)
{
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    const double spacing = 0.20;
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, 1.0, startX, startY);

    const double CW = 1.0 / 7.0;
    const double CH = 2.0 / 9.0;
    const double PW = CW * 0.85;
    const double PH = CH * 0.85;
    const double GU = (CW - PW) / 2.0;
    const double GV = (CH - PH) / 2.0;

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        const auto& pat = SANS_DIGITS[digits[i]];
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 7; ++col) {
                if (!pat[row][col]) continue;
                double x0 = col * CW + GU;
                double y0 = (8 - row) * CH + GV;
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

static FontGlyph fontSansImpl(size_t label, double scale, int maxDigits)
{
    FontGlyph g;
    g.engRects = buildSansRects(label, scale, maxDigits);
    auto ind = makeIndicatorBars(scale, maxDigits);
    g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    return g;
}

// ============================================================
//  7x9 Serif pixel font  (classic weighted serif numerals)
// ============================================================

// Same coordinate system as SANS_DIGITS.
// Key features: 2-pixel-wide strokes, explicit serif bars.
static const bool SERIF_DIGITS[10][9][7] = {
    // 0  — thick oval strokes
    {{ 0,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 0,1,1,1,1,1,0 }},
    // 1  — centered serif top, full-width base
    {{ 0,0,1,1,1,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 0,0,0,1,0,0,0 },
     { 1,1,1,1,1,1,1 }},
    // 2  — thick strokes + diagonal
    {{ 0,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,1,1,1,0 },
     { 0,0,1,1,0,0,0 },
     { 0,1,1,0,0,0,0 },
     { 1,1,0,0,0,0,0 },
     { 1,1,1,1,1,1,1 }},
    // 3  — thick right stroke, mid join
    {{ 0,1,1,1,1,1,0 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,1,1,1,1,0 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,1,1,1,1,1,0 }},
    // 4  — diagonal + thick stem + base serif
    {{ 0,0,0,0,1,1,0 },
     { 0,0,0,1,0,1,0 },
     { 0,0,1,0,0,1,0 },
     { 0,1,0,0,0,1,0 },
     { 1,0,0,0,0,1,0 },
     { 1,1,1,1,1,1,1 },
     { 0,0,0,0,0,1,0 },
     { 0,0,0,0,0,1,0 },
     { 0,0,0,0,1,1,1 }},
    // 5  — thick left/right strokes
    {{ 1,1,1,1,1,1,1 },
     { 1,1,0,0,0,0,0 },
     { 1,1,0,0,0,0,0 },
     { 1,1,0,0,0,0,0 },
     { 1,1,1,1,1,1,0 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 1,1,1,1,1,1,0 }},
    // 6  — curved entry, thick oval body
    {{ 0,0,1,1,1,1,0 },
     { 0,1,1,0,0,0,0 },
     { 1,1,0,0,0,0,0 },
     { 1,1,0,0,0,0,0 },
     { 1,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 0,1,1,1,1,1,0 }},
    // 7  — thick diagonal stroke + serif base
    {{ 1,1,1,1,1,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,1,1,0 },
     { 0,0,0,0,1,1,0 },
     { 0,0,0,1,1,0,0 },
     { 0,0,0,1,1,0,0 },
     { 0,0,1,1,0,0,0 },
     { 0,0,1,1,0,0,0 },
     { 0,1,1,1,1,0,0 }},
    // 8  — thick oval strokes
    {{ 0,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 0,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 0,1,1,1,1,1,0 }},
    // 9  — thick oval top, curved tail
    {{ 0,1,1,1,1,1,0 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 1,1,0,0,0,1,1 },
     { 0,1,1,1,1,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,0,1,1 },
     { 0,0,0,0,1,1,0 },
     { 0,1,1,1,1,0,0 }},
};

static std::vector<Rect2D> buildSerifRects(size_t label, double scale, int maxDigits)
{
    auto digits = labelDigits(label);
    int nd = (int)digits.size();
    const double spacing = 0.20;
    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, 1.0, startX, startY);

    const double CW = 1.0 / 7.0;
    const double CH = 2.0 / 9.0;
    const double PW = CW * 0.85;
    const double PH = CH * 0.85;
    const double GU = (CW - PW) / 2.0;
    const double GV = (CH - PH) / 2.0;

    std::vector<Rect2D> rects;
    for (int i = 0; i < nd; ++i) {
        double dx = i * (1.0 + spacing) * scale;
        const auto& pat = SERIF_DIGITS[digits[i]];
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 7; ++col) {
                if (!pat[row][col]) continue;
                double x0 = col * CW + GU;
                double y0 = (8 - row) * CH + GV;
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

static FontGlyph fontSerifImpl(size_t label, double scale, int maxDigits)
{
    FontGlyph g;
    g.engRects = buildSerifRects(label, scale, maxDigits);
    auto ind = makeIndicatorBars(scale, maxDigits);
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
        case FontStyle::Bold:
            return fontSeg7Impl(label, scale, maxDigits, 0.32, 0.01);
        case FontStyle::Narrow:
            return fontSeg7Impl(label, scale, maxDigits, 0.16, 0.02, 0.55, 0.15);
        case FontStyle::Pixel:
            return fontPixelImpl(label, scale, maxDigits);
        case FontStyle::HeavyPixel:
            return fontHeavyPixelImpl(label, scale, maxDigits);
        case FontStyle::SansSerif:
            return fontSansImpl(label, scale, maxDigits);
        case FontStyle::Serif:
            return fontSerifImpl(label, scale, maxDigits);
        case FontStyle::Blank:
            return FontGlyph{};
        case FontStyle::Seg7:
        default:
            return fontSeg7Impl(label, scale, maxDigits, 0.20, 0.03);
    }
}
