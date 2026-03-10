#pragma once
#include <vector>
#include "../Vec3.h"
#include "Common.h"

// Triangulate one die face with engraved digit segments cut into it.
// boundaryLoop : CCW boundary points on the cutting plane.
// center/normal/u/v : face coordinate frame.
// glyphRect    : uniform bounding rect (same on every face).
// engRects     : sub-rects to engrave (digit segments + orientation indicator).
// engraveDepth : channel depth in die coordinate units.
// draftAngleDeg: wall taper angle (degrees).
void createEngravedFace(const std::vector<Vec3>& boundaryLoop,
                        const Vec3& center, const Vec3& normal,
                        const Vec3& u,      const Vec3& v,
                        const Rect2D& glyphRect,
                        const std::vector<Rect2D>& engRects,
                        double engraveDepth,
                        double draftAngleDeg);
