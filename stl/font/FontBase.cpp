#include "../Font.h"
#include "../Engrave.h"
#include <algorithm>
#include <cmath>

std::vector<int> Font::labelDigits(size_t label) {
    std::vector<int> d;
    size_t n = label;
    do { d.push_back((int)(n % 10)); n /= 10; } while (n > 0);
    std::reverse(d.begin(), d.end());
    return d;
}

void Font::layoutMetrics(int nd, int maxDigits, double scale,
                         double spacing, double digitW,
                         double& startX, double& startY)
{
    double totalMaxW   = maxDigits * digitW + (maxDigits - 1) * spacing;
    double totalLabelW = nd        * digitW + (nd       - 1) * spacing;
    startX = (-(totalMaxW / 2.0) + (totalMaxW - totalLabelW) / 2.0) * scale;
    startY = -0.8 * scale;
}

bool Font::needsIndicator(size_t label, size_t sideCount) const {
    if (label % 10 == 0) return false;

    static const int basePair[10] = {0, -1, -1, -1, -1, -1, 9, -1, 8, 6};
    int pair[10];
    std::copy(std::begin(basePair), std::end(basePair), pair);
    if (oneIsSymmetric) pair[1] = 1;

    auto digits = labelDigits(label);
    for (int d : digits)
        if (pair[d] < 0) return false;

    size_t rotSym = 0;
    for (int d : digits)
        rotSym = rotSym * 10 + (size_t)pair[d];

    if (rotSym == label) return false;
    if (rotSym > sideCount) return false;

    return true;
}

std::vector<Rect2D> Font::indicatorBars(double hw, double scale) {
    double barH  = 0.085 * scale;
    double yBase = -1.12 * scale;
    return {
        { -hw*0.72, yBase + 2*barH,  hw*0.72, yBase + 3*barH },
        { -hw*0.46, yBase +   barH,  hw*0.46, yBase + 2*barH },
        { -hw*0.20, yBase,           hw*0.20, yBase +   barH },
    };
}

std::vector<Rect2D> Font::makeIndicatorBars(double scale, int maxDigits) {
    double hw = (maxDigits + (maxDigits - 1) * 0.20) / 2.0 * scale;
    return indicatorBars(hw, scale);
}

void Font::buildFace(size_t label, double scale, int maxDigits, size_t maxLabel,
                     const std::vector<Vec3>& loop,
                     const Vec3& center, const Vec3& normal,
                     const Vec3& u, const Vec3& v,
                     const Rect2D& glyphRect,
                     double engraveDepth, double draftAngleDeg) const
{
    auto glyph = build(label, scale, maxDigits, maxLabel);
    createEngravedFace(loop, center, normal, u, v,
                       glyphRect, glyph.engRects, glyph.engQuads,
                       glyph.engPolys, engraveDepth, draftAngleDeg);
}
