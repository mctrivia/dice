#pragma once
#include <vector>
#include "../Vec3.h"

// One flat face produced by clipping the sphere.
struct FaceData {
    std::vector<Vec3> loop;   // CCW boundary points on the cutting plane
    Vec3 center, normal, u, v;
};

// Build a sphere of radius r clipped to all die-face cutting planes.
// Emits sphere triangles via storeTriangle and returns the face boundary loops.
void buildDieSphere(double r, const std::vector<Vec3>& points,
                    std::vector<FaceData>& faces);
