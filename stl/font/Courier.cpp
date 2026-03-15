#include "Courier.h"
#include "../Engrave.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

using namespace std;

namespace {
    struct Point2D { double x, y; };
    using Loop = vector<Point2D>;
    using GlyphLoops = vector<Loop>;

#include "CourierData.inc"

    // ================================================================
    //  2D geometry helpers
    // ================================================================
    using Pt2  = pair<double,double>;
    using Poly = vector<Pt2>;

    // 2D cross product of vectors (A-O) × (B-O)
    double cross2(Pt2 O, Pt2 A, Pt2 B) {
        return (A.first-O.first)*(B.second-O.second)
             - (A.second-O.second)*(B.first-O.first);
    }

    // Point-in-polygon (ray casting)
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

    // Proper segment intersection (excludes endpoints)
    bool segsCross(Pt2 a, Pt2 b, Pt2 c, Pt2 d) {
        double d1 = cross2(c, d, a), d2 = cross2(c, d, b);
        double d3 = cross2(a, b, c), d4 = cross2(a, b, d);
        return ((d1 > 1e-10 && d2 < -1e-10) || (d1 < -1e-10 && d2 > 1e-10)) &&
               ((d3 > 1e-10 && d4 < -1e-10) || (d3 < -1e-10 && d4 > 1e-10));
    }

    // Check if diagonal (i,j) lies entirely inside polygon
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

    // Signed polygon area (positive = CCW, negative = CW)
    double polyArea(const Poly& p) {
        double a = 0;
        int n = (int)p.size();
        for (int i = 0; i < n; i++) {
            int j = (i+1) % n;
            a += p[i].first * p[j].second - p[j].first * p[i].second;
        }
        return a * 0.5;
    }

    void ensureCCW(Poly& p) { if (polyArea(p) < 0) reverse(p.begin(), p.end()); }
    void ensureCW(Poly& p)  { if (polyArea(p) > 0) reverse(p.begin(), p.end()); }

    // Point strictly inside triangle
    bool ptInTri(Pt2 P, Pt2 A, Pt2 B, Pt2 C) {
        double d1 = cross2(A,B,P), d2 = cross2(B,C,P), d3 = cross2(C,A,P);
        bool hasNeg = (d1 < -1e-10) || (d2 < -1e-10) || (d3 < -1e-10);
        bool hasPos = (d1 >  1e-10) || (d2 >  1e-10) || (d3 >  1e-10);
        return !(hasNeg && hasPos);
    }

    // Ear test for vertex i in a CCW polygon
    bool isEar(const Poly& p, int i) {
        int n = (int)p.size();
        int prev = (i-1+n)%n, next = (i+1)%n;
        Pt2 A = p[prev], B = p[i], C = p[next];
        if (cross2(A,B,C) <= 1e-10) return false;
        for (int j = 0; j < n; j++) {
            if (j == prev || j == i || j == next) continue;
            // Skip vertices coincident with triangle corners (bridge duplicates)
            const double eps = 1e-9;
            if (fabs(p[j].first-A.first)<eps && fabs(p[j].second-A.second)<eps) continue;
            if (fabs(p[j].first-B.first)<eps && fabs(p[j].second-B.second)<eps) continue;
            if (fabs(p[j].first-C.first)<eps && fabs(p[j].second-C.second)<eps) continue;
            if (ptInTri(p[j], A, B, C)) return false;
        }
        return true;
    }

    // Ear-clip a simple CCW polygon into triangles
    vector<array<Pt2,3>> earClip(Poly p) {
        vector<array<Pt2,3>> tris;
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
                // Fallback: split polygon with an interior diagonal, then
                // recursively ear-clip each half.
                bool split = false;
                for (int i = 0; i < n-2 && !split; i++) {
                    for (int j = i+2; j < n && !split; j++) {
                        if (i == 0 && j == n-1) continue; // adjacent
                        if (!isValidDiag(p, i, j)) continue;
                        // Split into two sub-polygons sharing diagonal (i,j)
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

                // Last resort: add a Steiner point at the centroid and
                // fan-triangulate only edges visible from it.
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

    // Merge a CW hole into a CCW outer polygon via bridge edge
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

        int nh = (int)hole.size();
        Poly merged;
        for (int i = 0; i <= candidate; i++) merged.push_back(outer[i]);
        for (int i = 0; i <= nh; i++)
            merged.push_back(hole[(hm+i) % nh]);
        merged.push_back(outer[candidate]);
        for (int i = candidate+1; i < no; i++) merged.push_back(outer[i]);
        return merged;
    }

    // Triangulate CCW outer polygon with CW holes
    vector<array<Pt2,3>> triWithHoles(Poly outer, vector<Poly> holes) {
        ensureCCW(outer);
        sort(holes.begin(), holes.end(), [](const Poly& a, const Poly& b){
            double ax = -1e18, bx = -1e18;
            for (auto& p : a) ax = max(ax, p.first);
            for (auto& p : b) bx = max(bx, p.first);
            return ax > bx;
        });
        for (auto& h : holes) {
            ensureCW(h);
            outer = mergeHole(outer, h);
        }
        return earClip(outer);
    }

    // ================================================================
    //  Polygon offset (shrink/expand by dist)
    //  dist > 0 = shrink inward, dist < 0 = expand outward
    //  Concave corners: intersection of offset edges (1 point)
    //  Convex corners: bevel with 2 points + short connecting wall
    // ================================================================
    struct OffsetResult {
        Poly floorPoly;
        vector<int> floorIdx;   // index of first floor vertex per original vertex
        vector<int> count;      // 1 or 2 per original vertex
    };

    OffsetResult offsetPoly(const Poly& poly, double dist) {
        int n = (int)poly.size();
        OffsetResult res;
        if (n < 3) { res.floorPoly = poly; return res; }

        double area = polyArea(poly);
        // For CW (area<0): sign=-1, inward normal = (dy,-dx)/len
        // For CCW (area>0): sign=+1, inward normal = (-dy,dx)/len
        double sign = (area > 0) ? 1.0 : -1.0;

        struct OEdge { Pt2 a, b; };
        vector<OEdge> oEdges(n);
        for (int i = 0; i < n; i++) {
            int j = (i+1) % n;
            double dx = poly[j].first  - poly[i].first;
            double dy = poly[j].second - poly[i].second;
            double len = sqrt(dx*dx + dy*dy);
            if (len < 1e-12) len = 1e-12;
            double nx = sign * (-dy) * dist / len;
            double ny = sign * ( dx) * dist / len;
            oEdges[i].a = {poly[i].first + nx, poly[i].second + ny};
            oEdges[i].b = {poly[j].first + nx, poly[j].second + ny};
        }

        res.floorIdx.resize(n);
        res.count.resize(n);

        for (int i = 0; i < n; i++) {
            int prev = (i-1+n) % n;
            Pt2 prevEnd   = oEdges[prev].b;
            Pt2 currStart = oEdges[i].a;

            double d1x = oEdges[prev].b.first  - oEdges[prev].a.first;
            double d1y = oEdges[prev].b.second - oEdges[prev].a.second;
            double d2x = oEdges[i].b.first  - oEdges[i].a.first;
            double d2y = oEdges[i].b.second - oEdges[i].a.second;
            double denom = d1x*d2y - d1y*d2x;

            res.floorIdx[i] = (int)res.floorPoly.size();

            if (fabs(denom) < 1e-12) {
                res.floorPoly.push_back({
                    (prevEnd.first + currStart.first) * 0.5,
                    (prevEnd.second + currStart.second) * 0.5
                });
                res.count[i] = 1;
            } else {
                double t = ((currStart.first - oEdges[prev].a.first)*d2y
                          - (currStart.second - oEdges[prev].a.second)*d2x) / denom;
                Pt2 intersection = {
                    oEdges[prev].a.first + t * d1x,
                    oEdges[prev].a.second + t * d1y
                };

                double mx = intersection.first  - poly[i].first;
                double my = intersection.second - poly[i].second;
                double miterDist = sqrt(mx*mx + my*my);

                if (miterDist > fabs(dist) * 4.0) {
                    res.floorPoly.push_back(prevEnd);
                    res.floorPoly.push_back(currStart);
                    res.count[i] = 2;
                } else {
                    res.floorPoly.push_back(intersection);
                    res.count[i] = 1;
                }
            }
        }
        return res;
    }

    // Font bounding box
    struct FontMetrics {
        double minX, maxX, minY, maxY;
        double width()  const { return maxX - minX; }
        double height() const { return maxY - minY; }
    };

    FontMetrics computeFontMetrics() {
        FontMetrics m = {1e18, -1e18, 1e18, -1e18};
        for (int d = 0; d < 10; d++)
            for (const auto& lp : _digitOutlines[d])
                for (const auto& pt : lp) {
                    m.minX = min(m.minX, pt.x);
                    m.maxX = max(m.maxX, pt.x);
                    m.minY = min(m.minY, pt.y);
                    m.maxY = max(m.maxY, pt.y);
                }
        return m;
    }

    // Transform a font Loop into face-local 2D coordinates
    Poly transformLoop(const Loop& loop, double fs,
                       double ox, double oy,
                       double fontMinX, double fontMinY) {
        Poly p;
        p.reserve(loop.size());
        for (const auto& pt : loop)
            p.push_back({ox + (pt.x - fontMinX) * fs,
                         oy + (pt.y - fontMinY) * fs});
        return p;
    }
}

// ================================================================
//  Courier::build — returns empty glyph (buildFace does the work)
// ================================================================
FontGlyph Courier::build(size_t, double, int, size_t) const {
    return FontGlyph{};
}

// ================================================================
//  Courier::buildFace — full face geometry with vector outlines
// ================================================================
void Courier::buildFace(size_t label, double scale, int maxDigits, size_t maxLabel,
                        const std::vector<Vec3>& faceLoop,
                        const Vec3& center, const Vec3& normal,
                        const Vec3& u, const Vec3& v,
                        const Rect2D& glyphRect,
                        double engraveDepth, double draftAngleDeg) const
{
    using Pt2 = pair<double,double>;

    double shrink = engraveDepth * tan(draftAngleDeg * M_PI / 180.0);
    Vec3 depthVec = normal * (-engraveDepth);

    auto to3D = [&](double lx, double ly) -> Vec3 {
        return center + u * lx + v * ly;
    };
    auto to3Dfloor = [&](double lx, double ly) -> Vec3 {
        return center + u * lx + v * ly + depthVec;
    };
    auto addTri = [&](Vec3 a, Vec3 b, Vec3 c) {
        if ((b-a).cross(c-a).dot(normal) < 0) swap(b, c);
        storeTriangle(normal, a, b, c);
    };

    // Wall triangle emitter: ensures normal faces expectedDir
    auto emitWallTri = [&](Vec3 a, Vec3 b, Vec3 c, const Vec3& expectedDir) {
        Vec3 wn = (b-a).cross(c-a);
        double len = wn.length();
        if (len < 1e-12) return;
        wn = wn * (1.0/len);
        if (wn.dot(expectedDir) < 0) {
            wn = wn * -1.0;
            swap(b, c);
        }
        storeTriangle(wn, a, b, c);
    };

    // Wall quad: fA→fB (face edge), flA→flB (floor edge), normal faces expectedDir
    auto addWall = [&](Vec3 fA, Vec3 fB, Vec3 flA, Vec3 flB,
                       const Vec3& expectedDir) {
        emitWallTri(fA, fB, flB, expectedDir);
        emitWallTri(fA, flB, flA, expectedDir);
    };

    // ============================================================
    //  Compute character positions with proper bounding box
    // ============================================================
    auto digits = labelDigits(label);
    int nd = (int)digits.size();

    FontMetrics fm = computeFontMetrics();
    double fontW = fm.width();
    double fontH = fm.height();
    if (fontW < 1e-9 || fontH < 1e-9) return;

    // Scale so character width = 1.0 in normalized units (matching glyph rect)
    double digitW = 1.0;
    double spacing = 0.20;
    double fs = digitW * scale / fontW;

    // Check height fits: character height in face units
    double charH = fontH * fs;
    // If character is too tall, scale down to fit
    double maxH = (glyphRect[3] - glyphRect[1]) * 0.85;
    if (charH > maxH) {
        fs *= maxH / charH;
        digitW = fontW * fs / scale;
        charH = fontH * fs;
    }

    double startX, startY;
    layoutMetrics(nd, maxDigits, scale, spacing, digitW, startX, startY);
    // Override startY to vertically center using the actual character height
    startY = -charH * 0.4;

    // Build transformed outlines for each digit
    struct DigitInfo {
        Poly outer;
        vector<Poly> islands;
    };
    vector<DigitInfo> digitInfos;
    for (int di = 0; di < nd; di++) {
        int d = digits[di];
        double ox = startX + di * (digitW + spacing) * scale;
        double oy = startY;
        DigitInfo info;
        const auto& loops = _digitOutlines[d];
        if (!loops.empty())
            info.outer = transformLoop(loops[0], fs, ox, oy, fm.minX, fm.minY);
        for (size_t li = 1; li < loops.size(); li++)
            info.islands.push_back(transformLoop(loops[li], fs, ox, oy, fm.minX, fm.minY));
        digitInfos.push_back(std::move(info));
    }

    // Indicator bar rectangles
    vector<Poly> indicatorPolys;
    if (needsIndicator(label, maxLabel)) {
        double hw = (maxDigits + (maxDigits - 1) * spacing) / 2.0 * scale;
        auto bars = indicatorBars(hw, scale);
        for (const auto& bar : bars) {
            // CW rectangle
            Poly rect = {
                {bar[0], bar[1]}, {bar[0], bar[3]},
                {bar[2], bar[3]}, {bar[2], bar[1]}
            };
            indicatorPolys.push_back(rect);
        }
    }

    // Padded glyph rect — used by both zipper (1d) and face fill (1a)
    double faceRadius = 0;
    for (const auto& pt : faceLoop)
        faceRadius = max(faceRadius, (pt - center).length());
    double padding = faceRadius * 0.02;
    double bx0 = glyphRect[0]-padding, by0 = glyphRect[1]-padding;
    double bx1 = glyphRect[2]+padding, by1 = glyphRect[3]+padding;

    // ============================================================
    //  SUB-TASK 1d: ZIPPER — face boundary loop → glyph rect
    // ============================================================
    // #define STOP_1D
    #ifndef STOP_1D
    {
        vector<Pt2> rectPts = {{bx0,by0},{bx1,by0},{bx1,by1},{bx0,by1}};
        const int N = 4;
        int M = (int)faceLoop.size();
        vector<Pt2>    circlePts(M);
        vector<double> circleAng(M);
        for (int i = 0; i < M; i++) {
            Vec3 d = faceLoop[i] - center;
            circlePts[i] = {d.dot(u), d.dot(v)};
            circleAng[i] = atan2(circlePts[i].second, circlePts[i].first);
        }
        array<double,4> rectAng;
        for (int i = 0; i < N; i++)
            rectAng[i] = atan2(rectPts[i].second, rectPts[i].first);

        auto ccwDist = [](double from, double to) {
            double d = to - from;
            while (d < 0)         d += 2*M_PI;
            while (d >= 2*M_PI)   d -= 2*M_PI;
            return d;
        };

        int ci0 = 0;
        { double best = 1e18;
          for (int i = 0; i < M; i++) {
              double d1 = ccwDist(rectAng[0], circleAng[i]);
              double d2 = ccwDist(circleAng[i], rectAng[0]);
              if (min(d1,d2) < best) { best = min(d1,d2); ci0 = i; }
          }
        }

        int ci = ci0, ri = 0, ci_used = 0, ri_used = 0;
        for (int step = 0; step < M+N; step++) {
            int ci_next = (ci+1)%M, ri_next = (ri+1)%N;
            Pt2 C0 = circlePts[ci], R0 = rectPts[ri];
            bool advRect;
            if      (ri_used >= N) advRect = false;
            else if (ci_used >= M) advRect = true;
            else advRect = (ccwDist(circleAng[ci], rectAng[ri_next])
                         <= ccwDist(circleAng[ci], circleAng[ci_next]));
            if (advRect) {
                addTri(to3D(C0.first,C0.second),
                       to3D(R0.first,R0.second),
                       to3D(rectPts[ri_next].first,rectPts[ri_next].second));
                ri = ri_next; ri_used++;
            } else {
                addTri(to3D(C0.first,C0.second),
                       to3D(R0.first,R0.second),
                       to3D(circlePts[ci_next].first,circlePts[ci_next].second));
                ci = ci_next; ci_used++;
            }
        }
    }
    #endif // STOP_1D

    // ============================================================
    //  SUB-TASK 1a: FACE SURFACE with character holes
    // ============================================================
    // #define STOP_1A
    #ifndef STOP_1A
    {
        // Use the same padded rect as the zipper target
        Poly outer = {
            {bx0, by0}, {bx1, by0}, {bx1, by1}, {bx0, by1},
        };

        vector<Poly> holes;
        for (const auto& di : digitInfos)
            if (!di.outer.empty())
                holes.push_back(di.outer);
        for (const auto& ip : indicatorPolys)
            holes.push_back(ip);

        if (holes.empty()) {
            addTri(to3D(bx0, by0), to3D(bx1, by0), to3D(bx1, by1));
            addTri(to3D(bx0, by0), to3D(bx1, by1), to3D(bx0, by1));
        } else {
            auto tris = triWithHoles(outer, holes);
            for (const auto& tri : tris)
                addTri(to3D(tri[0].first, tri[0].second),
                       to3D(tri[1].first, tri[1].second),
                       to3D(tri[2].first, tri[2].second));
        }
    }
    #endif // STOP_1A

    // ============================================================
    //  SUB-TASK 1b: ISLAND CAPS (solid face-level surfaces)
    // ============================================================
    // #define STOP_1B
    #ifndef STOP_1B
    for (const auto& di : digitInfos) {
        for (const auto& island : di.islands) {
            Poly isl = island;
            ensureCCW(isl);
            auto tris = earClip(isl);
            for (const auto& tri : tris)
                addTri(to3D(tri[0].first, tri[0].second),
                       to3D(tri[1].first, tri[1].second),
                       to3D(tri[2].first, tri[2].second));
        }
    }
    #endif // STOP_1B

    // ============================================================
    //  SUB-TASK 1c: FLOOR + WALLS of engraved channels
    // ============================================================
    // #define STOP_1C
    #ifndef STOP_1C
    {
        for (const auto& di : digitInfos) {
            if (di.outer.empty()) continue;
            const Poly& faceOuter = di.outer;
            int nOuter = (int)faceOuter.size();

            // Offset outer loop INWARD for the floor (pocket narrows at floor)
            OffsetResult outerOff = offsetPoly(faceOuter, shrink);
            const Poly& floorOuter = outerOff.floorPoly;

            // --- Outer walls ---
            // For CW outer loop: right-perpendicular points inward (toward channel)
            // Wall normal should point INWARD = toward channel center
            for (int i = 0; i < nOuter; i++) {
                int j = (i+1) % nOuter;
                Vec3 fA = to3D(faceOuter[i].first, faceOuter[i].second);
                Vec3 fB = to3D(faceOuter[j].first, faceOuter[j].second);

                int fi_last = outerOff.floorIdx[i] + outerOff.count[i] - 1;
                int fj_first = outerOff.floorIdx[j];
                Vec3 flA = to3Dfloor(floorOuter[fi_last].first,
                                     floorOuter[fi_last].second);
                Vec3 flB = to3Dfloor(floorOuter[fj_first].first,
                                     floorOuter[fj_first].second);

                // Compute expected wall direction: right-perpendicular of edge
                // For CW polygon, right-perp = inward toward channel
                double dx = faceOuter[j].first  - faceOuter[i].first;
                double dy = faceOuter[j].second - faceOuter[i].second;
                Vec3 expectedDir = u * dy - v * dx;  // right perpendicular in 3D
                addWall(fA, fB, flA, flB, expectedDir);
            }

            // Convex corner caps on outer loop
            for (int i = 0; i < nOuter; i++) {
                if (outerOff.count[i] != 2) continue;
                int fi0 = outerOff.floorIdx[i];
                int fi1 = fi0 + 1;
                Vec3 faceV = to3D(faceOuter[i].first, faceOuter[i].second);
                Vec3 flP0  = to3Dfloor(floorOuter[fi0].first,
                                       floorOuter[fi0].second);
                Vec3 flP1  = to3Dfloor(floorOuter[fi1].first,
                                       floorOuter[fi1].second);
                // Cap faces inward: use vector from face vertex toward polygon centroid
                int prev = (i-1+nOuter)%nOuter;
                double dx = faceOuter[i].first  - faceOuter[prev].first;
                double dy = faceOuter[i].second - faceOuter[prev].second;
                Vec3 expectedDir = u * dy - v * dx;
                emitWallTri(faceV, flP0, flP1, expectedDir);
            }

            // --- Island walls ---
            // Islands EXPAND at floor (island base is wider with draft angle)
            vector<Poly> floorIslands;
            for (const auto& island : di.islands) {
                int nIsland = (int)island.size();
                // Negative shrink = expand outward
                OffsetResult islOff = offsetPoly(island, -shrink);
                const Poly& floorIsland = islOff.floorPoly;
                floorIslands.push_back(floorIsland);

                // Island walls: normal faces OUTWARD from island (into channel)
                // For CW island traversal, LEFT-perpendicular = outward from island
                for (int i = 0; i < nIsland; i++) {
                    int j = (i+1) % nIsland;
                    Vec3 fA = to3D(island[i].first, island[i].second);
                    Vec3 fB = to3D(island[j].first, island[j].second);
                    int fi_last = islOff.floorIdx[i] + islOff.count[i] - 1;
                    int fj_first = islOff.floorIdx[j];
                    Vec3 flA = to3Dfloor(floorIsland[fi_last].first,
                                         floorIsland[fi_last].second);
                    Vec3 flB = to3Dfloor(floorIsland[fj_first].first,
                                         floorIsland[fj_first].second);

                    // Left perpendicular = outward from CW island
                    double dx = island[j].first  - island[i].first;
                    double dy = island[j].second - island[i].second;
                    Vec3 expectedDir = u * (-dy) + v * dx;  // left perp
                    addWall(fA, fB, flA, flB, expectedDir);
                }
                // Convex corner caps on island
                for (int i = 0; i < nIsland; i++) {
                    if (islOff.count[i] != 2) continue;
                    int fi0 = islOff.floorIdx[i];
                    int fi1 = fi0 + 1;
                    Vec3 faceV = to3D(island[i].first, island[i].second);
                    Vec3 flP0  = to3Dfloor(floorIsland[fi0].first,
                                           floorIsland[fi0].second);
                    Vec3 flP1  = to3Dfloor(floorIsland[fi1].first,
                                           floorIsland[fi1].second);
                    int prev = (i-1+nIsland)%nIsland;
                    double dx = island[i].first  - island[prev].first;
                    double dy = island[i].second - island[prev].second;
                    Vec3 expectedDir = u * (-dy) + v * dx;
                    emitWallTri(faceV, flP0, flP1, expectedDir);
                }
            }

            // --- Floor triangulation ---
            // Floor is inside the shrunk outer loop, outside the expanded island loops
            Poly floorOuterCCW = floorOuter;
            ensureCCW(floorOuterCCW);
            vector<Poly> floorHoles;
            for (auto& fi : floorIslands) {
                Poly h = fi;
                ensureCW(h);
                floorHoles.push_back(h);
            }
            auto floorTris = triWithHoles(floorOuterCCW, floorHoles);
            for (const auto& tri : floorTris)
                addTri(to3Dfloor(tri[0].first, tri[0].second),
                       to3Dfloor(tri[1].first, tri[1].second),
                       to3Dfloor(tri[2].first, tri[2].second));
        }

        // --- Indicator bars: simple rect engraving ---
        for (const auto& bar : indicatorPolys) {
            OffsetResult barOff = offsetPoly(bar, shrink);
            const Poly& floorBar = barOff.floorPoly;
            int nb = (int)bar.size();

            for (int i = 0; i < nb; i++) {
                int j = (i+1) % nb;
                Vec3 fA = to3D(bar[i].first, bar[i].second);
                Vec3 fB = to3D(bar[j].first, bar[j].second);
                int fi_last = barOff.floorIdx[i] + barOff.count[i] - 1;
                int fj_first = barOff.floorIdx[j];
                Vec3 flA = to3Dfloor(floorBar[fi_last].first,
                                     floorBar[fi_last].second);
                Vec3 flB = to3Dfloor(floorBar[fj_first].first,
                                     floorBar[fj_first].second);
                double dx = bar[j].first  - bar[i].first;
                double dy = bar[j].second - bar[i].second;
                Vec3 expectedDir = u * dy - v * dx;  // right perp (CW rect)
                addWall(fA, fB, flA, flB, expectedDir);
            }
            for (int i = 0; i < nb; i++) {
                if (barOff.count[i] != 2) continue;
                int fi0 = barOff.floorIdx[i];
                int fi1 = fi0 + 1;
                Vec3 faceV = to3D(bar[i].first, bar[i].second);
                Vec3 flP0  = to3Dfloor(floorBar[fi0].first,
                                       floorBar[fi0].second);
                Vec3 flP1  = to3Dfloor(floorBar[fi1].first,
                                       floorBar[fi1].second);
                int prev = (i-1+nb)%nb;
                double dx = bar[i].first  - bar[prev].first;
                double dy = bar[i].second - bar[prev].second;
                Vec3 expectedDir = u * dy - v * dx;
                emitWallTri(faceV, flP0, flP1, expectedDir);
            }

            Poly floorBarCCW = floorBar;
            ensureCCW(floorBarCCW);
            auto floorTris = earClip(floorBarCCW);
            for (const auto& tri : floorTris)
                addTri(to3Dfloor(tri[0].first, tri[0].second),
                       to3Dfloor(tri[1].first, tri[1].second),
                       to3Dfloor(tri[2].first, tri[2].second));
        }
    }
    #endif // STOP_1C
}
