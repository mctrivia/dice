#include "Engrave.h"
#include <cmath>
#include <algorithm>
#include <limits>

void createEngravedFace(const std::vector<Vec3>& boundaryLoop,
                        const Vec3& center, const Vec3& normal,
                        const Vec3& u,      const Vec3& v,
                        const Rect2D& glyphRect,
                        const std::vector<Rect2D>& engRects,
                        double engraveDepth,
                        double draftAngleDeg)
{
    using Pt2 = std::pair<double, double>;

    if (boundaryLoop.size() < 3) {
        size_t n = boundaryLoop.size();
        for (size_t i = 0; i < n; ++i) {
            Vec3 v0 = boundaryLoop[i], v1 = boundaryLoop[(i+1)%n], v2 = center;
            if ((v1-v0).cross(v2-v0).dot(normal) < 0) std::swap(v0, v1);
            storeTriangle(normal, v0, v1, v2);
        }
        return;
    }

    // Ensure correct winding then store.
    auto addTri = [&](Vec3 a, Vec3 b, Vec3 c) {
        if ((b-a).cross(c-a).dot(normal) < 0) std::swap(b, c);
        storeTriangle(normal, a, b, c);
    };

    auto to3D = [&](double lx, double ly) -> Vec3 {
        return center + u * lx + v * ly;
    };

    // -----------------------------------------------------------------------
    // Zipper triangulation: circle boundary → rectangular hole
    // -----------------------------------------------------------------------
    double faceRadius = 0;
    for (const auto& pt : boundaryLoop)
        faceRadius = std::max(faceRadius, (pt - center).length());
    double padding = faceRadius * 0.02;
    double bx0 = glyphRect[0] - padding, by0 = glyphRect[1] - padding;
    double bx1 = glyphRect[2] + padding, by1 = glyphRect[3] + padding;

    std::vector<Pt2> rectPts = {
        {bx0, by0}, {bx1, by0}, {bx1, by1}, {bx0, by1}
    };
    const int N = 4;

    int M = (int)boundaryLoop.size();
    std::vector<Pt2>    circlePts(M);
    std::vector<double> circleAng(M);
    for (int i = 0; i < M; ++i) {
        Vec3 d = boundaryLoop[i] - center;
        circlePts[i] = {d.dot(u), d.dot(v)};
        circleAng[i] = atan2(circlePts[i].second, circlePts[i].first);
    }

    std::array<double, 4> rectAng;
    for (int i = 0; i < N; ++i)
        rectAng[i] = atan2(rectPts[i].second, rectPts[i].first);

    auto ccwDist = [](double from, double to) -> double {
        double d = to - from;
        while (d <  0)         d += 2 * M_PI;
        while (d >= 2 * M_PI)  d -= 2 * M_PI;
        return d;
    };

    int ci0 = 0;
    {
        double best = 1e18, r0a = rectAng[0];
        for (int i = 0; i < M; ++i) {
            double d  = ccwDist(r0a, circleAng[i]);
            double d2 = ccwDist(circleAng[i], r0a);
            double sym = std::min(d, d2);
            if (sym < best) { best = sym; ci0 = i; }
        }
    }

    int ci = ci0, ri = 0, ci_used = 0, ri_used = 0;
    for (int step = 0; step < M + N; ++step) {
        int ci_next = (ci + 1) % M;
        int ri_next = (ri + 1) % N;
        Pt2 C0 = circlePts[ci], R0 = rectPts[ri];
        Pt2 C1 = circlePts[ci_next], R1 = rectPts[ri_next];

        bool advanceRect;
        if      (ri_used >= N) advanceRect = false;
        else if (ci_used >= M) advanceRect = true;
        else {
            double ref = circleAng[ci];
            double dC  = ccwDist(ref, circleAng[ci_next]);
            double dR  = ccwDist(ref, rectAng[ri_next]);
            advanceRect = (dR <= dC);
        }

        if (advanceRect) {
            addTri(to3D(C0.first, C0.second),
                   to3D(R0.first, R0.second),
                   to3D(R1.first, R1.second));
            ri = ri_next; ri_used++;
        } else {
            addTri(to3D(C0.first, C0.second),
                   to3D(R0.first, R0.second),
                   to3D(C1.first, C1.second));
            ci = ci_next; ci_used++;
        }
        (void)C1; (void)R1; // suppress unused-variable warning
    }

    if (engRects.empty()) return;

    // -----------------------------------------------------------------------
    // Fill the rectangle interior at face level, cutting out the engRects.
    // -----------------------------------------------------------------------
    std::vector<double> xs = {bx0, bx1};
    std::vector<double> ys = {by0, by1};
    for (const auto& er : engRects) {
        xs.push_back(std::max(bx0, std::min(bx1, er[0])));
        xs.push_back(std::max(bx0, std::min(bx1, er[2])));
        ys.push_back(std::max(by0, std::min(by1, er[1])));
        ys.push_back(std::max(by0, std::min(by1, er[3])));
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    int nxs = (int)xs.size(), nys = (int)ys.size();
    for (int ix = 0; ix + 1 < nxs; ++ix) {
        for (int iy = 0; iy + 1 < nys; ++iy) {
            double cx = (xs[ix] + xs[ix+1]) * 0.5;
            double cy = (ys[iy] + ys[iy+1]) * 0.5;
            bool inEngr = false;
            for (const auto& er : engRects)
                if (cx > er[0] && cx < er[2] && cy > er[1] && cy < er[3])
                    { inEngr = true; break; }
            if (inEngr) continue;
            Vec3 p00 = to3D(xs[ix],   ys[iy]  );
            Vec3 p10 = to3D(xs[ix+1], ys[iy]  );
            Vec3 p11 = to3D(xs[ix+1], ys[iy+1]);
            Vec3 p01 = to3D(xs[ix],   ys[iy+1]);
            addTri(p00, p10, p11);
            addTri(p00, p11, p01);
        }
    }

    // -----------------------------------------------------------------------
    // Engraved rect: floor + 4 draft-angled side walls.
    // -----------------------------------------------------------------------
    Vec3 depthVec = normal * (-engraveDepth);
    double shrink = engraveDepth * std::tan(draftAngleDeg * M_PI / 180.0);

    // Sub-spans of [lo,hi] with the given ranges cut out.
    auto subSpans = [](double lo, double hi,
                       std::vector<std::pair<double,double>> cuts)
        -> std::vector<std::pair<double,double>>
    {
        std::sort(cuts.begin(), cuts.end());
        std::vector<std::pair<double,double>> result;
        const double eps = 1e-9;
        double cur = lo;
        for (auto [a, b] : cuts) {
            if (a > cur + eps) result.push_back({cur, std::min(a, hi)});
            cur = std::max(cur, b);
            if (cur >= hi - eps) break;
        }
        if (cur < hi - eps) result.push_back({cur, hi});
        return result;
    };

    // Wall quad: face-level edge fA→fB with matching floor points.
    auto addSubWall = [&](Vec3 fA, Vec3 fB, Vec3 flA, Vec3 flB) {
        Vec3 wn = (fB - fA).cross(flA - fA);
        double len = wn.length();
        if (len < 1e-12) return;
        wn = wn * (1.0 / len);
        if ((fB - fA).cross(flB - fA).dot(wn) < 0) { std::swap(fA, fB); std::swap(flA, flB); }
        storeTriangle(wn, fA, fB, flB);
        storeTriangle(wn, fA, flB, flA);
    };

    // Shrink applied to floor x/y coord only at the rect's own boundary edge.
    auto uShrink = [&](double x, double rx0, double rx1) {
        const double eps = 1e-9;
        if (fabs(x - rx0) < eps) return  shrink;
        if (fabs(x - rx1) < eps) return -shrink;
        return 0.0;
    };
    auto vShrink = [&](double y, double ry0, double ry1) {
        const double eps = 1e-9;
        if (fabs(y - ry0) < eps) return  shrink;
        if (fabs(y - ry1) < eps) return -shrink;
        return 0.0;
    };

    for (const auto& er : engRects) {
        double x0 = er[0], y0 = er[1], x1 = er[2], y1 = er[3];
        const double eps = 1e-9;

        // Floor
        Vec3 Af = to3D(x0,y0) + depthVec + u* shrink + v* shrink;
        Vec3 Bf = to3D(x1,y0) + depthVec + u*-shrink + v* shrink;
        Vec3 Cf = to3D(x1,y1) + depthVec + u*-shrink + v*-shrink;
        Vec3 Df = to3D(x0,y1) + depthVec + u* shrink + v*-shrink;
        addTri(Af, Bf, Cf);
        addTri(Af, Cf, Df);

        // Bottom wall (y = y0, normal ~+v)
        {
            std::vector<std::pair<double,double>> cuts;
            for (const auto& other : engRects) {
                if (&other == &er) continue;
                if (fabs(other[3] - y0) < eps) {
                    double a = std::max(x0, other[0]), b = std::min(x1, other[2]);
                    if (b > a + eps) cuts.push_back({a, b});
                }
            }
            for (auto [xa, xb] : subSpans(x0, x1, cuts)) {
                Vec3 fA = to3D(xa, y0), fB = to3D(xb, y0);
                Vec3 flA = to3D(xa + uShrink(xa,x0,x1), y0 + shrink) + depthVec;
                Vec3 flB = to3D(xb + uShrink(xb,x0,x1), y0 + shrink) + depthVec;
                addSubWall(fA, fB, flA, flB);
            }
        }

        // Top wall (y = y1, normal ~-v)
        {
            std::vector<std::pair<double,double>> cuts;
            for (const auto& other : engRects) {
                if (&other == &er) continue;
                if (fabs(other[1] - y1) < eps) {
                    double a = std::max(x0, other[0]), b = std::min(x1, other[2]);
                    if (b > a + eps) cuts.push_back({a, b});
                }
            }
            for (auto [xa, xb] : subSpans(x0, x1, cuts)) {
                Vec3 fA = to3D(xb, y1), fB = to3D(xa, y1);
                Vec3 flA = to3D(xb + uShrink(xb,x0,x1), y1 - shrink) + depthVec;
                Vec3 flB = to3D(xa + uShrink(xa,x0,x1), y1 - shrink) + depthVec;
                addSubWall(fA, fB, flA, flB);
            }
        }

        // Left wall (x = x0, normal ~+u)
        {
            std::vector<std::pair<double,double>> cuts;
            for (const auto& other : engRects) {
                if (&other == &er) continue;
                if (fabs(other[2] - x0) < eps) {
                    double a = std::max(y0, other[1]), b = std::min(y1, other[3]);
                    if (b > a + eps) cuts.push_back({a, b});
                }
            }
            for (auto [ya, yb] : subSpans(y0, y1, cuts)) {
                Vec3 fA = to3D(x0, yb), fB = to3D(x0, ya);
                Vec3 flA = to3D(x0 + shrink, yb + vShrink(yb,y0,y1)) + depthVec;
                Vec3 flB = to3D(x0 + shrink, ya + vShrink(ya,y0,y1)) + depthVec;
                addSubWall(fA, fB, flA, flB);
            }
        }

        // Right wall (x = x1, normal ~-u)
        {
            std::vector<std::pair<double,double>> cuts;
            for (const auto& other : engRects) {
                if (&other == &er) continue;
                if (fabs(other[0] - x1) < eps) {
                    double a = std::max(y0, other[1]), b = std::min(y1, other[3]);
                    if (b > a + eps) cuts.push_back({a, b});
                }
            }
            for (auto [ya, yb] : subSpans(y0, y1, cuts)) {
                Vec3 fA = to3D(x1, ya), fB = to3D(x1, yb);
                Vec3 flA = to3D(x1 - shrink, ya + vShrink(ya,y0,y1)) + depthVec;
                Vec3 flB = to3D(x1 - shrink, yb + vShrink(yb,y0,y1)) + depthVec;
                addSubWall(fA, fB, flA, flB);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Step corners + floor strip: close staircase gaps at junctions where
    // adjacent bars share a horizontal edge but have different widths.
    // -----------------------------------------------------------------------
    auto floorXAt = [&](double x, double rx0, double rx1) -> double {
        const double ep = 1e-9;
        if (fabs(x - rx0) < ep) return x + shrink;
        if (fabs(x - rx1) < ep) return x - shrink;
        return x;
    };

    for (size_t i = 0; i < engRects.size(); ++i) {
        for (size_t j = 0; j < engRects.size(); ++j) {
            if (i == j) continue;
            const auto& lower = engRects[i];
            const auto& upper = engRects[j];
            if (fabs(lower[3] - upper[1]) > 1e-9) continue;  // not a shared edge
            double y_s  = lower[3];
            double x_lo = std::max(lower[0], upper[0]);
            double x_hi = std::min(lower[2], upper[2]);
            if (x_hi <= x_lo + 1e-9) continue;               // no x overlap

            // Side triangles at each step corner (x_lo and x_hi).
            for (double x_c : {x_lo, x_hi}) {
                Vec3 face_pt  = to3D(x_c, y_s);
                Vec3 lower_fl = to3D(floorXAt(x_c, lower[0], lower[2]), y_s - shrink) + depthVec;
                Vec3 upper_fl = to3D(floorXAt(x_c, upper[0], upper[2]), y_s + shrink) + depthVec;

                Vec3 wn = (lower_fl - face_pt).cross(upper_fl - face_pt);
                double len = wn.length();
                if (len < 1e-12) continue;
                wn = wn * (1.0 / len);

                double cx = (lower[0] + lower[2]) * 0.5;
                Vec3 outward = (x_c > cx) ? (u * -1.0) : u;
                if (wn.dot(outward) < 0) {
                    wn = wn * -1.0;
                    storeTriangle(wn, face_pt, upper_fl, lower_fl);
                } else {
                    storeTriangle(wn, face_pt, lower_fl, upper_fl);
                }
            }

            // Floor strip: trapezoid at depth connecting the two floors.
            Vec3 BL_f = to3D(floorXAt(x_lo, lower[0], lower[2]), y_s - shrink) + depthVec;
            Vec3 BR_f = to3D(floorXAt(x_hi, lower[0], lower[2]), y_s - shrink) + depthVec;
            Vec3 TR_f = to3D(floorXAt(x_hi, upper[0], upper[2]), y_s + shrink) + depthVec;
            Vec3 TL_f = to3D(floorXAt(x_lo, upper[0], upper[2]), y_s + shrink) + depthVec;
            addTri(BL_f, BR_f, TR_f);
            addTri(BL_f, TR_f, TL_f);
        }
    }
}
