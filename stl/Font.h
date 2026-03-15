#pragma once
#include <vector>
#include <array>
#include <memory>
#include "Common.h"

// Engraved geometry for one die face label.
struct FontGlyph {
    std::vector<Rect2D> engRects;
    std::vector<Quad2D> engQuads;
    std::vector<Poly2D> engPolys;
};

// ============================================================
//  Base class — shared drawing utilities
// ============================================================
struct Font {
    bool oneIsSymmetric = false;

    virtual FontGlyph build(size_t label, double scale, int maxDigits, size_t sideCount) const = 0;
    virtual int maxSides() const { return 0; }

    // Build the complete face geometry (triangles via storeTriangle).
    // Default: calls build() then createEngravedFace().
    // Override for fonts that use a different face-building strategy.
    virtual void buildFace(size_t label, double scale, int maxDigits, size_t maxLabel,
                           const std::vector<Vec3>& loop,
                           const Vec3& center, const Vec3& normal,
                           const Vec3& u, const Vec3& v,
                           const Rect2D& glyphRect,
                           double engraveDepth, double draftAngleDeg) const;

    virtual ~Font() = default;

protected:
    static std::vector<int> labelDigits(size_t label);
    static void layoutMetrics(int nd, int maxDigits, double scale,
                               double spacing, double digitW,
                               double& startX, double& startY);
    bool needsIndicator(size_t label, size_t sideCount) const;
    static std::vector<Rect2D> indicatorBars(double hw, double scale);
    static std::vector<Rect2D> makeIndicatorBars(double scale, int maxDigits);
};

// ============================================================
//  Concrete font types
// ============================================================

// Simplified 7-segment font: rectangular segments, no gaps between segments.
// sw = stroke width, spacing = inter-digit gap.
struct Seg7Font : Font {
    double sw = 0.20, spacing = 0.20;
    Seg7Font() = default;
    Seg7Font(double sw, double spacing = 0.20) : sw(sw), spacing(spacing) {}
    FontGlyph build(size_t label, double scale, int maxDigits, size_t sideCount) const override;
private:
    std::array<Rect2D, 7> makeSegs() const;
};

// 5x7 dot-matrix pixel font.
struct PixelFont : Font {
    FontGlyph build(size_t label, double scale, int maxDigits, size_t sideCount) const override;
private:
    static std::vector<Rect2D> buildPixelRects(size_t label, double scale, int maxDigits);
    static std::vector<Rect2D> makeIndicatorPixel(double scale, int maxDigits);
};

// 4x6 large blocky pixel font.
struct HeavyPixelFont : Font {
    FontGlyph build(size_t label, double scale, int maxDigits, size_t sideCount) const override;
private:
    static std::vector<Rect2D> buildHeavyRects(size_t label, double scale, int maxDigits);
};

// No engraving — smooth faces.
struct BlankFont : Font {
    FontGlyph build(size_t, double, int, size_t) const override { return FontGlyph{}; }
};

// ============================================================
//  Preset factory
// ============================================================

enum class FontStyle {
    Seg7, Pixel, HeavyPixel, Blank, Courier,
};

std::unique_ptr<Font> makeFontDef(FontStyle style);
