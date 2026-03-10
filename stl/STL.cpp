#include "STL.h"
#include "Common.h"
#include "Sphere.h"
#include "Font.h"
#include "Engrave.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <limits>

// ---------------------------------------------------------------------------
// Triangle accumulator (definition used by Sphere.cpp and Engrave.cpp)
// ---------------------------------------------------------------------------

static std::vector<std::array<Vec3, 4>> _triangles;

void storeTriangle(const Vec3& normal, const Vec3& v1, const Vec3& v2, const Vec3& v3) {
    _triangles.push_back({normal, v1, v2, v3});
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void createSTL(double r, const std::vector<Vec3>& points,
               const std::string& fileName, const std::vector<size_t>& labels,
               FontStyle font, double engraveDepth, double draftAngleDeg)
{
    _triangles.clear();

    // Build clipped sphere and collect face boundary loops.
    std::vector<FaceData> faces;
    buildDieSphere(r, points, faces);

    // Compute uniform glyph rect (same size on every face).
    const bool doEngrave = !labels.empty();
    Rect2D uniformGlyphRect = {};
    double uniformScale = 0;
    int maxDigits = 1;

    if (doEngrave) {
        size_t maxLabel = *std::max_element(labels.begin(), labels.end());
        maxDigits = (maxLabel >= 10) ? (int)(floor(log10((double)maxLabel)) + 1) : 1;

        double minFaceRadius = std::numeric_limits<double>::max();
        for (const auto& fd : faces) {
            double fr = 0;
            for (const auto& pt : fd.loop)
                fr = std::max(fr, (pt - fd.center).length());
            minFaceRadius = std::min(minFaceRadius, fr);
        }

        const double spacing = 0.20;
        double totalW  = maxDigits + (maxDigits - 1) * spacing;
        double halfW   = totalW / 2.0;
        double diagHalf = sqrt(halfW * halfW + 1.2 * 1.2);
        uniformScale = (minFaceRadius * 0.97) / diagHalf;

        uniformGlyphRect = {
            -halfW * uniformScale,
            -1.2   * uniformScale,
             halfW * uniformScale,
             1.2   * uniformScale
        };
    }

    // Draw face caps.
    for (size_t f = 0; f < faces.size(); ++f) {
        const auto& fd = faces[f];
        if (doEngrave && f < labels.size()) {
            double depthUnits = (r > 1e-9) ? engraveDepth * fd.center.length() / r : engraveDepth;
            auto glyph = buildGlyph(font, labels[f], uniformScale, maxDigits);
            createEngravedFace(fd.loop, fd.center, fd.normal, fd.u, fd.v,
                               uniformGlyphRect, glyph.engRects, depthUnits, draftAngleDeg);
        } else {
            size_t nv = fd.loop.size();
            for (size_t i = 0; i < nv; ++i) {
                Vec3 v0 = fd.loop[i], v1 = fd.loop[(i+1)%nv], v2 = fd.center;
                if ((v1-v0).cross(v2-v0).dot(fd.normal) < 0) std::swap(v0, v1);
                storeTriangle(fd.normal, v0, v1, v2);
            }
        }
    }

    // Write binary STL.
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open()) { std::cerr << "Cannot open " << fileName << "\n"; return; }

    char header[80] = {};
    std::string title = "Binary STL die";
    std::memcpy(header, title.c_str(), std::min(title.size(), (size_t)79));
    file.write(header, 80);

    uint32_t triCount = (uint32_t)_triangles.size();
    file.write(reinterpret_cast<char*>(&triCount), 4);

    for (const auto& tri : _triangles) {
        for (int i = 0; i < 4; ++i) {
            float fx = (float)tri[i].x, fy = (float)tri[i].y, fz = (float)tri[i].z;
            file.write(reinterpret_cast<char*>(&fx), 4);
            file.write(reinterpret_cast<char*>(&fy), 4);
            file.write(reinterpret_cast<char*>(&fz), 4);
        }
        uint16_t attr = 0;
        file.write(reinterpret_cast<char*>(&attr), 2);
    }
    file.close();
    std::cout << "Wrote " << _triangles.size() << " triangles to " << fileName << "\n";
}

double computeMaxRadius(const std::vector<Vec3>& points) {
    if (points.size() < 2) return -1.0;

    double minDist = std::numeric_limits<double>::max();
    size_t idx1 = 0, idx2 = 1;
    for (size_t i = 0; i < points.size(); i += 2)
        for (size_t j = i+1; j < points.size(); ++j) {
            double d = (points[i]-points[j]).length();
            if (d < minDist) { minDist = d; idx1 = i; idx2 = j; }
        }

    Vec3 P1 = points[idx1], P2 = points[idx2];
    double d1 = P1.length(), d2 = P2.length();
    if (d1 == 0 || d2 == 0) return -1.0;

    Vec3 n1 = P1, n2 = P2;
    n1.normalize(); n2.normalize();

    Vec3 num = (n1 * d2) - (n2 * d1);
    Vec3 den = n1.cross(n2);
    if (den.length() < 1e-6) return -1.0;
    return num.length() / den.length();
}
