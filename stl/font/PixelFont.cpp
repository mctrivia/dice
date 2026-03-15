#include "../Font.h"

// 5x7 pixel font data
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

std::vector<Rect2D> PixelFont::buildPixelRects(size_t label, double scale, int maxDigits)
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

std::vector<Rect2D> PixelFont::makeIndicatorPixel(double scale, int maxDigits)
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

FontGlyph PixelFont::build(size_t label, double scale, int maxDigits, size_t sideCount) const
{
    FontGlyph g;
    g.engRects = buildPixelRects(label, scale, maxDigits);
    if (needsIndicator(label, sideCount)) {
        auto ind = makeIndicatorPixel(scale, maxDigits);
        g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    }
    return g;
}

// ============================================================
//  HeavyPixelFont
// ============================================================

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

std::vector<Rect2D> HeavyPixelFont::buildHeavyRects(size_t label, double scale, int maxDigits)
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

FontGlyph HeavyPixelFont::build(size_t label, double scale, int maxDigits, size_t sideCount) const
{
    FontGlyph g;
    g.engRects = buildHeavyRects(label, scale, maxDigits);
    if (needsIndicator(label, sideCount)) {
        auto ind = makeIndicatorBars(scale, maxDigits);
        g.engRects.insert(g.engRects.end(), ind.begin(), ind.end());
    }
    return g;
}
