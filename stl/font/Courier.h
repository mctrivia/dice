#include "stl/Font.h"

struct Courier : Font {
    FontGlyph build(size_t label, double scale, int maxDigits, size_t sideCount) const override;

    void buildFace(size_t label, double scale, int maxDigits, size_t maxLabel,
                   const std::vector<Vec3>& loop,
                   const Vec3& center, const Vec3& normal,
                   const Vec3& u, const Vec3& v,
                   const Rect2D& glyphRect,
                   double engraveDepth, double draftAngleDeg) const override;
};
