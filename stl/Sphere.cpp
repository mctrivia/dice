#include "Sphere.h"
#include "Common.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <array>

static void clipPolygonWithPlane(const std::vector<Vec3>& polygon, const Plane& plane,
                                  std::vector<Vec3>& out, std::vector<Vec3>& intersections) {
    out.clear();
    size_t n = polygon.size();
    for (size_t i = 0; i < n; ++i) {
        const Vec3& cur  = polygon[i];
        const Vec3& prev = polygon[(i + n - 1) % n];
        double cd = plane.distance(cur), pd = plane.distance(prev);
        bool ci = cd <= 0, pi = pd <= 0;
        if (ci) {
            if (!pi) { Vec3 p = plane.intersect(prev, cur); out.push_back(p); intersections.push_back(p); }
            out.push_back(cur);
        } else if (pi) {
            Vec3 p = plane.intersect(prev, cur); out.push_back(p); intersections.push_back(p);
        }
    }
}

void buildDieSphere(double r, const std::vector<Vec3>& points,
                    std::vector<FaceData>& faces)
{
    const int latDiv = 100, lonDiv = 100;

    // Sphere vertices
    std::vector<Vec3> vertices;
    vertices.reserve((latDiv+1)*(lonDiv+1));
    for (int i = 0; i <= latDiv; ++i) {
        double theta = i * M_PI / latDiv;
        for (int j = 0; j <= lonDiv; ++j) {
            double phi = j * 2 * M_PI / lonDiv;
            vertices.push_back(Vec3(r * sin(theta)*cos(phi),
                                    r * sin(theta)*sin(phi),
                                    r * cos(theta)));
        }
    }

    // Sphere face indices
    std::vector<std::array<int,3>> sphereFaces;
    sphereFaces.reserve(latDiv * lonDiv * 2);
    for (int i = 0; i < latDiv; ++i)
        for (int j = 0; j < lonDiv; ++j) {
            int f = i*(lonDiv+1)+j, s = f+lonDiv+1;
            sphereFaces.push_back({f, s, f+1});
            sphereFaces.push_back({s, s+1, f+1});
        }

    // Cutting planes (one per die face point)
    struct CuttingPlane { Plane _plane; Vec3 _point; };
    std::vector<CuttingPlane> cuttingPlanes;
    for (const Vec3& pt : points) {
        Vec3 n = pt; n.normalize();
        cuttingPlanes.push_back({Plane(n, n.dot(pt)), pt});
    }

    std::vector<std::vector<Vec3>> planeBoundaries(cuttingPlanes.size());

    // Clip every sphere triangle against all cutting planes
    for (const auto& fi : sphereFaces) {
        std::vector<Vec3> clipped = {vertices[fi[0]], vertices[fi[1]], vertices[fi[2]]};
        for (size_t p = 0; p < cuttingPlanes.size(); ++p) {
            std::vector<Vec3> tmp, ints;
            clipPolygonWithPlane(clipped, cuttingPlanes[p]._plane, tmp, ints);
            planeBoundaries[p].insert(planeBoundaries[p].end(), ints.begin(), ints.end());
            clipped = tmp;
            if (clipped.empty()) break;
        }
        if (clipped.size() < 3) continue;
        for (size_t i = 1; i+1 < clipped.size(); ++i) {
            Vec3 e1 = clipped[i] - clipped[0], e2 = clipped[i+1] - clipped[0];
            Vec3 fn = e1.cross(e2); fn.normalize();
            storeTriangle(fn, clipped[0], clipped[i], clipped[i+1]);
        }
    }

    // Build face boundary loops (deduplicated, sorted by angle)
    faces.clear();
    faces.reserve(cuttingPlanes.size());
    for (size_t p = 0; p < cuttingPlanes.size(); ++p) {
        auto& ips = planeBoundaries[p];
        if (ips.empty()) continue;

        std::vector<Vec3> bpts;
        std::unordered_set<Vec3, Vec3Hash, Vec3Equal> seen;
        for (const auto& pt : ips)
            if (seen.find(pt) == seen.end()) { bpts.push_back(pt); seen.insert(pt); }

        Vec3 normal = cuttingPlanes[p]._plane._normal;
        Vec3 center = cuttingPlanes[p]._point;

        Vec3 u;
        if (fabs(normal.x) > 1e-6 || fabs(normal.y) > 1e-6)
            u = Vec3(-normal.y, normal.x, 0.0);
        else
            u = Vec3(1.0, 0.0, 0.0);
        u.normalize();
        Vec3 v = normal.cross(u);

        std::vector<std::pair<double, Vec3>> av;
        av.reserve(bpts.size());
        for (const auto& pt : bpts) {
            Vec3 d = pt - center;
            av.emplace_back(atan2(d.dot(v), d.dot(u)), pt);
        }
        std::sort(av.begin(), av.end());
        std::vector<Vec3> loop;
        loop.reserve(av.size());
        for (const auto& a : av) loop.push_back(a.second);

        faces.push_back({loop, center, normal, u, v});
    }
}
