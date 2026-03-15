#include "Engrave.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <array>

namespace {
    using Pt2  = std::pair<double,double>;
    using Poly = std::vector<Pt2>;

    double cross2(Pt2 O, Pt2 A, Pt2 B) {
        return (A.first-O.first)*(B.second-O.second)
             - (A.second-O.second)*(B.first-O.first);
    }

    bool pointInPoly(Pt2 pt, const Poly& poly) {
        int n = (int)poly.size();
        bool inside = false;
        for (int i = 0, j = n-1; i < n; j = i++) {
            if (((poly[i].second > pt.second) != (poly[j].second > pt.second)) &&
                (pt.first < (poly[j].first - poly[i].first) *
                 (pt.second - poly[i].second) /
                 (poly[j].second - poly[i].second) + poly[i].first))
                inside = !inside;
        }
        return inside;
    }

    bool segsCross(Pt2 a, Pt2 b, Pt2 c, Pt2 d) {
        double d1 = cross2(c, d, a), d2 = cross2(c, d, b);
        double d3 = cross2(a, b, c), d4 = cross2(a, b, d);
        return ((d1 > 1e-10 && d2 < -1e-10) || (d1 < -1e-10 && d2 > 1e-10)) &&
               ((d3 > 1e-10 && d4 < -1e-10) || (d3 < -1e-10 && d4 > 1e-10));
    }

    bool isValidDiag(const Poly& p, int i, int j) {
        int n = (int)p.size();
        for (int k = 0; k < n; k++) {
            int l = (k+1) % n;
            if (k == i || k == j || l == i || l == j) continue;
            if (segsCross(p[i], p[j], p[k], p[l])) return false;
        }
        Pt2 mid = {(p[i].first+p[j].first)*0.5, (p[i].second+p[j].second)*0.5};
        return pointInPoly(mid, p);
    }

    double polyArea(const Poly& p) {
        double a = 0;
        int n = (int)p.size();
        for (int i = 0; i < n; i++) {
            int j = (i+1) % n;
            a += p[i].first * p[j].second - p[j].first * p[i].second;
        }
        return a * 0.5;
    }

    void ensureCCW(Poly& p) { if (polyArea(p) < 0) std::reverse(p.begin(), p.end()); }
    void ensureCW(Poly& p)  { if (polyArea(p) > 0) std::reverse(p.begin(), p.end()); }

    bool ptInTri(Pt2 P, Pt2 A, Pt2 B, Pt2 C) {
        double d1 = cross2(A,B,P), d2 = cross2(B,C,P), d3 = cross2(C,A,P);
        bool hasNeg = (d1 < -1e-10) || (d2 < -1e-10) || (d3 < -1e-10);
        bool hasPos = (d1 >  1e-10) || (d2 >  1e-10) || (d3 >  1e-10);
        return !(hasNeg && hasPos);
    }

    bool isEar(const Poly& p, int i) {
        int n = (int)p.size();
        int prev = (i-1+n)%n, next = (i+1)%n;
        Pt2 A = p[prev], B = p[i], C = p[next];
        if (cross2(A,B,C) <= 1e-10) return false;
        for (int j = 0; j < n; j++) {
            if (j == prev || j == i || j == next) continue;
            const double eps = 1e-9;
            if (fabs(p[j].first-A.first)<eps && fabs(p[j].second-A.second)<eps) continue;
            if (fabs(p[j].first-B.first)<eps && fabs(p[j].second-B.second)<eps) continue;
            if (fabs(p[j].first-C.first)<eps && fabs(p[j].second-C.second)<eps) continue;
            if (ptInTri(p[j], A, B, C)) return false;
        }
        return true;
    }

    std::vector<std::array<Pt2,3>> earClip(Poly p) {
        std::vector<std::array<Pt2,3>> tris;
        while ((int)p.size() > 3) {
            int n = (int)p.size();
            bool found = false;
            for (int i = 0; i < n; i++) {
                if (isEar(p, i)) {
                    int prev = (i-1+n)%n, next = (i+1)%n;
                    tris.push_back({p[prev], p[i], p[next]});
                    p.erase(p.begin()+i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                bool split = false;
                for (int i = 0; i < n-2 && !split; i++) {
                    for (int j = i+2; j < n && !split; j++) {
                        if (i == 0 && j == n-1) continue;
                        if (!isValidDiag(p, i, j)) continue;
                        Poly p1, p2;
                        for (int k = i; k <= j; k++) p1.push_back(p[k]);
                        for (int k = j; k != i; k = (k+1)%n) p2.push_back(p[k]);
                        p2.push_back(p[i]);
                        auto t1 = earClip(p1);
                        auto t2 = earClip(p2);
                        tris.insert(tris.end(), t1.begin(), t1.end());
                        tris.insert(tris.end(), t2.begin(), t2.end());
                        split = true;
                    }
                }
                if (split) return tris;

                double cx = 0, cy = 0;
                for (auto& pt : p) { cx += pt.first; cy += pt.second; }
                cx /= n; cy /= n;
                Pt2 S = {cx, cy};
                for (int i = 0; i < n; i++) {
                    int j = (i+1) % n;
                    if (cross2(p[i], p[j], S) > 1e-12)
                        tris.push_back({p[i], p[j], S});
                }
                return tris;
            }
        }
        if ((int)p.size() == 3)
            tris.push_back({p[0], p[1], p[2]});
        return tris;
    }

    Poly mergeHole(const Poly& outer, const Poly& hole) {
        int hm = 0;
        for (int i = 1; i < (int)hole.size(); i++)
            if (hole[i].first > hole[hm].first ||
                (hole[i].first == hole[hm].first && hole[i].second > hole[hm].second))
                hm = i;

        Pt2 M = hole[hm];
        int no = (int)outer.size();

        double bestX = 1e18;
        int bestEdge = -1;
        for (int i = 0; i < no; i++) {
            int j = (i+1) % no;
            Pt2 A = outer[i], B = outer[j];
            double dy = B.second - A.second;
            if (fabs(dy) < 1e-12) continue;
            double t = (M.second - A.second) / dy;
            if (t < -1e-9 || t > 1.0+1e-9) continue;
            double ix = A.first + t * (B.first - A.first);
            if (ix < M.first - 1e-9) continue;
            if (ix < bestX) { bestX = ix; bestEdge = i; }
        }

        if (bestEdge < 0) {
            double bestD = 1e18;
            int bestV = 0;
            for (int i = 0; i < no; i++) {
                double dx = outer[i].first - M.first;
                double dy2 = outer[i].second - M.second;
                double d = dx*dx + dy2*dy2;
                if (d < bestD) { bestD = d; bestV = i; }
            }
            Poly merged;
            for (int i = 0; i <= bestV; i++) merged.push_back(outer[i]);
            for (int i = 0; i < (int)hole.size(); i++)
                merged.push_back(hole[(hm+i) % (int)hole.size()]);
            merged.push_back(hole[hm]);
            merged.push_back(outer[bestV]);
            for (int i = bestV+1; i < no; i++) merged.push_back(outer[i]);
            return merged;
        }

        int edgeA = bestEdge, edgeB = (bestEdge+1) % no;
        int candidate = (outer[edgeA].first >= outer[edgeB].first) ? edgeA : edgeB;

        Pt2 P = {bestX, M.second};
        for (int i = 0; i < no; i++) {
            int prev = (i-1+no)%no, next = (i+1)%no;
            if (cross2(outer[prev], outer[i], outer[next]) >= 0) continue;
            if (!ptInTri(outer[i], M, P, outer[candidate])) continue;
            double dx = outer[i].first - M.first;
            double dy2 = fabs(outer[i].second - M.second);
            double ang = atan2(dy2, dx);
            double cdx = outer[candidate].first - M.first;
            double cdy = fabs(outer[candidate].second - M.second);
            double cang = atan2(cdy, cdx);
            if (ang < cang) candidate = i;
        }

        Poly merged;
        for (int i = 0; i <= candidate; i++) merged.push_back(outer[i]);
        for (int i = 0; i < (int)hole.size(); i++)
            merged.push_back(hole[(hm+i) % (int)hole.size()]);
        merged.push_back(hole[hm]);
        merged.push_back(outer[candidate]);
        for (int i = candidate+1; i < no; i++) merged.push_back(outer[i]);
        return merged;
    }

    std::vector<std::array<Pt2,3>> triWithHoles(Poly outer, std::vector<Poly> holes) {
        ensureCCW(outer);
        std::sort(holes.begin(), holes.end(), [](const Poly& a, const Poly& b){
            double ax = -1e18, bx = -1e18;
            for (auto& p : a) ax = std::max(ax, p.first);
            for (auto& p : b) bx = std::max(bx, p.first);
            return ax > bx;
        });
        for (auto& h : holes) {
            ensureCW(h);
            outer = mergeHole(outer, h);
        }
        return earClip(outer);
    }
} // anonymous namespace

void createEngravedFace(const std::vector<Vec3>& boundaryLoop,
                        const Vec3& center, const Vec3& normal,
                        const Vec3& u,      const Vec3& v,
                        const Rect2D& glyphRect,
                        const std::vector<Rect2D>& engRects,
                        const std::vector<Quad2D>& engQuads,
                        const std::vector<Poly2D>& engPolys,
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

    if (engRects.empty() && engQuads.empty() && engPolys.empty()) {
        // No engraving: fill the rectangle with just 2 triangles and done.
        addTri(to3D(bx0, by0), to3D(bx1, by0), to3D(bx1, by1));
        addTri(to3D(bx0, by0), to3D(bx1, by1), to3D(bx0, by1));
        return;
    }

    // -----------------------------------------------------------------------
    // Fill the rectangle interior at face level, cutting out engravings.
    // Uses a fine grid with point-in-polygon tests for circular pip cutouts.
    // -----------------------------------------------------------------------
    std::vector<double> xs = {bx0, bx1};
    std::vector<double> ys = {by0, by1};
    for (const auto& er : engRects) {
        xs.push_back(std::max(bx0, std::min(bx1, er[0])));
        xs.push_back(std::max(bx0, std::min(bx1, er[2])));
        ys.push_back(std::max(by0, std::min(by1, er[1])));
        ys.push_back(std::max(by0, std::min(by1, er[3])));
    }
    for (const auto& eq : engQuads) {
        double qxmin = std::min({eq[0],eq[2],eq[4],eq[6]});
        double qxmax = std::max({eq[0],eq[2],eq[4],eq[6]});
        double qymin = std::min({eq[1],eq[3],eq[5],eq[7]});
        double qymax = std::max({eq[1],eq[3],eq[5],eq[7]});
        xs.push_back(std::max(bx0, std::min(bx1, qxmin)));
        xs.push_back(std::max(bx0, std::min(bx1, qxmax)));
        ys.push_back(std::max(by0, std::min(by1, qymin)));
        ys.push_back(std::max(by0, std::min(by1, qymax)));
    }
    // Add all pip polygon vertex coordinates as grid breakpoints for
    // fine resolution around circular cutouts.
    for (const auto& ep : engPolys) {
        int pn = (int)ep.size() / 2;
        for (int k = 0; k < pn; ++k) {
            xs.push_back(std::max(bx0, std::min(bx1, ep[2*k])));
            ys.push_back(std::max(by0, std::min(by1, ep[2*k+1])));
        }
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    // Point-in-convex-quad test (works for both CW and CCW winding).
    auto pointInQuad = [](double px, double py, const Quad2D& q) -> bool {
        double orient = (q[2]-q[0])*(q[5]-q[1]) - (q[3]-q[1])*(q[4]-q[0]);
        for (int k = 0; k < 4; ++k) {
            double ax = q[2*k], ay = q[2*k+1];
            double bx = q[(2*k+2)%8], by = q[(2*k+3)%8];
            double cross = (bx-ax)*(py-ay) - (by-ay)*(px-ax);
            if ((orient > 0 && cross < -1e-9) || (orient < 0 && cross > 1e-9)) return false;
        }
        return true;
    };

    // Point-in-polygon via ray casting for circular pip cutouts.
    auto pointInEngPoly = [](double px, double py, const Poly2D& ep) -> bool {
        int pn = (int)ep.size() / 2;
        bool inside = false;
        for (int i = 0, j = pn-1; i < pn; j = i++) {
            double xi = ep[2*i], yi = ep[2*i+1];
            double xj = ep[2*j], yj = ep[2*j+1];
            if (((yi > py) != (yj > py)) &&
                (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
                inside = !inside;
        }
        return inside;
    };

    int nxs = (int)xs.size(), nys = (int)ys.size();
    for (int ix = 0; ix + 1 < nxs; ++ix) {
        for (int iy = 0; iy + 1 < nys; ++iy) {
            double cx = (xs[ix] + xs[ix+1]) * 0.5;
            double cy = (ys[iy] + ys[iy+1]) * 0.5;
            bool inEngr = false;
            for (const auto& er : engRects)
                if (cx > er[0] && cx < er[2] && cy > er[1] && cy < er[3])
                    { inEngr = true; break; }
            if (!inEngr)
                for (const auto& eq : engQuads)
                    if (pointInQuad(cx, cy, eq)) { inEngr = true; break; }
            if (!inEngr)
                for (const auto& ep : engPolys)
                    if (pointInEngPoly(cx, cy, ep)) { inEngr = true; break; }
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
    // Convex polygons: floor (fan triangulation) + draft-angled side walls.
    // Floor is shrunk inward by 'shrink' (centroid-based) to match draft angle.
    // -----------------------------------------------------------------------
    for (const auto& ep : engPolys) {
        int pn = (int)ep.size() / 2;

        // Centroid of the face polygon.
        double pcx = 0, pcy = 0;
        for (int k = 0; k < pn; ++k) { pcx += ep[2*k]; pcy += ep[2*k+1]; }
        pcx /= pn; pcy /= pn;

        std::vector<Vec3> P(pn), Pf(pn);
        for (int k = 0; k < pn; ++k) {
            P[k] = to3D(ep[2*k], ep[2*k+1]);
            // Shrink floor vertex toward centroid by 'shrink' units.
            double vx = ep[2*k] - pcx, vy = ep[2*k+1] - pcy;
            double len = std::sqrt(vx*vx + vy*vy);
            double sf = (len > shrink) ? (len - shrink) / len : 0.0;
            Pf[k] = to3D(pcx + vx*sf, pcy + vy*sf) + depthVec;
        }

        // Floor: fan from first vertex.
        for (int k = 1; k + 1 < pn; ++k)
            addTri(Pf[0], Pf[k], Pf[k+1]);

        // Walls: forward traversal (CCW polygon → outward-facing normals).
        for (int k = 0; k < pn; ++k)
            addSubWall(P[k], P[(k+1)%pn], Pf[k], Pf[(k+1)%pn]);
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

    // -----------------------------------------------------------------------
    // Diagonal quads: floor + straight walls (no shrink / draft for simplicity).
    // -----------------------------------------------------------------------
    for (const auto& eq : engQuads) {
        Vec3 P[4], Pf[4];
        for (int k = 0; k < 4; ++k) {
            P[k]  = to3D(eq[2*k], eq[2*k+1]);
            Pf[k] = P[k] + depthVec;
        }
        // Floor
        addTri(Pf[0], Pf[1], Pf[2]);
        addTri(Pf[0], Pf[2], Pf[3]);
        // Four walls — reverse traversal so outward normals face away from interior.
        for (int k = 0; k < 4; ++k)
            addSubWall(P[(k+1)%4], P[k], Pf[(k+1)%4], Pf[k]);
    }

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
