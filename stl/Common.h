#pragma once
#include <array>
#include <cmath>
#include "../Vec3.h"

// Axis-aligned rectangle in face-local 2D coords: {x0, y0, x1, y1}.
using Rect2D = std::array<double, 4>;

struct Vec3Hash {
    std::size_t operator()(const Vec3& v) const {
        std::hash<double> h;
        return h(v.x) ^ (h(v.y) << 1) ^ (h(v.z) << 2);
    }
};

struct Vec3Equal {
    bool operator()(const Vec3& a, const Vec3& b) const {
        const double eps = 1e-6;
        return fabs(a.x-b.x)<eps && fabs(a.y-b.y)<eps && fabs(a.z-b.z)<eps;
    }
};

struct Plane {
    Vec3 _normal;
    double _d;
    Plane(const Vec3& n, double d) : _normal(n), _d(d) {}
    double distance(const Vec3& p) const { return _normal.dot(p) - _d; }
    Vec3 intersect(const Vec3& v0, const Vec3& v1) const {
        double d0 = distance(v0), d1 = distance(v1);
        double t = d0 / (d0 - d1);
        return v0 + (v1 - v0) * t;
    }
};

// Triangle accumulator — defined in STL.cpp, used by Sphere and Engrave.
void storeTriangle(const Vec3& normal, const Vec3& v1, const Vec3& v2, const Vec3& v3);
