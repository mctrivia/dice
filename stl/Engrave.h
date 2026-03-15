#pragma once
#include <vector>
#include "../Vec3.h"
#include "Common.h"

// Triangulate one die face with engraved digit segments cut into it.
// boundaryLoop : CCW boundary points on the cutting plane.
// center/normal/u/v : face coordinate frame.
// glyphRect    : uniform bounding rect (same on every face).
// engRects     : axis-aligned sub-rects to engrave.
// engQuads     : arbitrary quads for diagonal strokes.
// engPolys     : convex polygons (hex 7-seg segments, circular pips).
// engraveDepth : channel depth in die coordinate units.
// draftAngleDeg: wall taper angle (degrees).
void createEngravedFace(const std::vector<Vec3>& boundaryLoop,
                        const Vec3& center, const Vec3& normal,
                        const Vec3& u,      const Vec3& v,
                        const Rect2D& glyphRect,
                        const std::vector<Rect2D>& engRects,
                        const std::vector<Quad2D>& engQuads,
                        const std::vector<Poly2D>& engPolys,
                        double engraveDepth,
                        double draftAngleDeg);
