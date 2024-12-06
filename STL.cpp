//
// Created by mctrivia on 17/09/24.
//
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <cstring>
#include "STL.h"

using namespace std;

struct Vec3Hash {
    std::size_t operator()(const Vec3& v) const {
        std::hash<double> hasher;
        std::size_t h1 = hasher(v.x);
        std::size_t h2 = hasher(v.y);
        std::size_t h3 = hasher(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct Vec3Equal {
    bool operator()(const Vec3& a, const Vec3& b) const {
        double epsilon = 1e-6;
        return std::abs(a.x - b.x) < epsilon &&
               std::abs(a.y - b.y) < epsilon &&
               std::abs(a.z - b.z) < epsilon;
    }
};

struct Plane {
    Vec3 _normal;
    double _d; // plane equation: normal.dot(point) = d

    Plane(const Vec3& normal_, double d_) : _normal(normal_), _d(d_) {}

    double distance(const Vec3& point) const {
        return _normal.dot(point) - _d;
    }

    Vec3 intersect(const Vec3& v0, const Vec3& v1) const {
        double dist0 = distance(v0);
        double dist1 = distance(v1);
        double t = dist0 / (dist0 - dist1);
        return v0 + (v1 - v0) * t;
    }
};

// We will store all triangles here and write them at the end
// Each triangle: [0] = normal, [1] = v0, [2] = v1, [3] = v2
static std::vector<std::array<Vec3,4>> _triangles;

// Store a triangle for later writing in binary format
void storeTriangle(const Vec3& normal, const Vec3& v1, const Vec3& v2, const Vec3& v3) {
    std::array<Vec3,4> tri = {normal, v1, v2, v3};
    _triangles.push_back(tri);
}

// Function to clip a polygon (triangle) against a plane and collect intersection points
void clipPolygonWithPlane(const std::vector<Vec3>& polygon, const Plane& plane, std::vector<Vec3>& outPolygon,
                          std::vector<Vec3>& intersectionPoints) {
    outPolygon.clear();
    size_t n = polygon.size();
    for (size_t i = 0; i < n; ++i) {
        const Vec3& currVertex = polygon[i];
        const Vec3& prevVertex = polygon[(i + n - 1) % n];
        double currDist = plane.distance(currVertex);
        double prevDist = plane.distance(prevVertex);

        bool currInside = currDist <= 0;
        bool prevInside = prevDist <= 0;

        if (currInside) {
            if (!prevInside) {
                Vec3 intersectPoint = plane.intersect(prevVertex, currVertex);
                outPolygon.push_back(intersectPoint);
                intersectionPoints.push_back(intersectPoint);
            }
            outPolygon.push_back(currVertex);
        } else if (prevInside) {
            Vec3 intersectPoint = plane.intersect(prevVertex, currVertex);
            outPolygon.push_back(intersectPoint);
            intersectionPoints.push_back(intersectPoint);
        }
    }
}

void createSTL(double r, const std::vector<Vec3>& points, const std::string& fileName) {
    _triangles.clear(); // Clear any previously stored triangles

    // Number of divisions for latitude and longitude
    const int latDiv = 100;
    const int lonDiv = 100;

    // Generate sphere vertices
    std::vector<Vec3> vertices;
    vertices.reserve((latDiv+1)*(lonDiv+1));
    for (int i = 0; i <= latDiv; ++i) {
        double theta = i * M_PI / latDiv;
        double sinTheta = sin(theta);
        double cosTheta = cos(theta);

        for (int j = 0; j <= lonDiv; ++j) {
            double phi = j * 2 * M_PI / lonDiv;
            double sinPhi = sin(phi);
            double cosPhi = cos(phi);

            Vec3 v(
                    r * sinTheta * cosPhi,
                    r * sinTheta * sinPhi,
                    r * cosTheta
            );
            vertices.push_back(v);
        }
    }

    // Generate sphere faces (triangles)
    std::vector<std::array<int, 3>> faces;
    faces.reserve(latDiv*lonDiv*2);
    for (int i = 0; i < latDiv; ++i) {
        for (int j = 0; j < lonDiv; ++j) {
            int first = i * (lonDiv + 1) + j;
            int second = first + lonDiv + 1;

            faces.push_back({first, second, first + 1});
            faces.push_back({second, second + 1, first + 1});
        }
    }

    struct CuttingPlane {
        Plane _plane;
        Vec3 _point;
    };
    std::vector<CuttingPlane> cuttingPlanes;

    for (const Vec3& point: points) {
        Vec3 normal = point;
        normal.normalize();
        double d = normal.dot(point);
        cuttingPlanes.push_back({Plane(normal, d), point});
    }

    // For each cutting plane, maintain a list of intersection points
    std::vector<std::vector<Vec3>> planeBoundaries(cuttingPlanes.size());

    // Process each face and apply the cuts
    for (const auto& faceIndices: faces) {
        std::vector<Vec3> polygon = {
                vertices[faceIndices[0]],
                vertices[faceIndices[1]],
                vertices[faceIndices[2]]
        };

        std::vector<Vec3> clippedPolygon = polygon;

        // For each cutting plane
        for (size_t p = 0; p < cuttingPlanes.size(); ++p) {
            std::vector<Vec3> tempPolygon;
            std::vector<Vec3> intersectionPoints;
            clipPolygonWithPlane(clippedPolygon, cuttingPlanes[p]._plane, tempPolygon, intersectionPoints);

            planeBoundaries[p].insert(planeBoundaries[p].end(), intersectionPoints.begin(), intersectionPoints.end());

            clippedPolygon = tempPolygon;

            if (clippedPolygon.empty()) {
                break;
            }
        }

        if (clippedPolygon.size() < 3) {
            continue;
        }

        // Triangulate the polygon (assuming convexity)
        for (size_t i = 1; i + 1 < clippedPolygon.size(); ++i) {
            Vec3 v0 = clippedPolygon[0];
            Vec3 v1 = clippedPolygon[i];
            Vec3 v2 = clippedPolygon[i + 1];

            // Compute normal
            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            Vec3 faceNormal = edge1.cross(edge2);
            faceNormal.normalize();

            // Store triangle
            storeTriangle(faceNormal, v0, v1, v2);
        }
    }

    // For each cutting plane, create caps
    for (size_t p = 0; p < cuttingPlanes.size(); ++p) {
        auto& intersectionPoints = planeBoundaries[p];

        if (intersectionPoints.empty()) {
            continue;
        }

        // Remove duplicates
        std::vector<Vec3> boundaryPoints;
        std::unordered_set<Vec3, Vec3Hash, Vec3Equal> pointSet;
        for (const auto& pt: intersectionPoints) {
            if (pointSet.find(pt) == pointSet.end()) {
                boundaryPoints.push_back(pt);
                pointSet.insert(pt);
            }
        }

        Vec3 normal = cuttingPlanes[p]._plane._normal;
        Vec3 center = cuttingPlanes[p]._point;

        Vec3 u;
        if (fabs(normal.x) > 1e-6 || fabs(normal.y) > 1e-6) {
            u = Vec3(-normal.y, normal.x, 0).normalize();
        } else {
            u = Vec3(1, 0, 0);
        }
        Vec3 v = normal.cross(u);

        // Compute angles
        std::vector<std::pair<double, Vec3>> anglePointPairs;
        for (const auto& vertex: boundaryPoints) {
            Vec3 vec = vertex - center;
            double x = vec.dot(u);
            double y = vec.dot(v);
            double angle = atan2(y, x);
            anglePointPairs.emplace_back(angle, vertex);
        }

        // Sort by angle
        std::sort(anglePointPairs.begin(), anglePointPairs.end());

        // Rebuild boundary loop
        std::vector<Vec3> boundaryLoop;
        for (const auto& ap: anglePointPairs) {
            boundaryLoop.push_back(ap.second);
        }

        // Triangulate the cap
        size_t numVertices = boundaryLoop.size();
        for (size_t i = 0; i < numVertices; ++i) {
            Vec3 v0 = boundaryLoop[i];
            Vec3 v1 = boundaryLoop[(i + 1) % numVertices];
            Vec3 v2 = center;

            // Ensure correct orientation
            if ((v1 - v0).cross(v2 - v0).dot(normal) < 0) {
                std::swap(v0, v1);
            }

            storeTriangle(normal, v0, v1, v2);
        }
    }

    // Now write the binary STL file
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file " << fileName << " for writing.\n";
        return;
    }

    // 80-byte header (can be anything)
    char header[80] = {};
    std::string title = "Binary STL sphere";
    std::memcpy(header, title.c_str(), std::min<size_t>(title.size(),79));
    file.write(header, 80);

    // Number of triangles
    uint32_t triCount = static_cast<uint32_t>(_triangles.size());
    file.write(reinterpret_cast<char*>(&triCount), sizeof(triCount));

    // Write each triangle
    // STL requires floats, so convert double to float
    for (const auto& tri : _triangles) {
        // tri[0] = normal, tri[1..3] = vertices
        // normal
        for (int i = 0; i < 4; i++) {
            const Vec3& v = tri[i];
            float fx = (float)v.x;
            float fy = (float)v.y;
            float fz = (float)v.z;
            file.write(reinterpret_cast<char*>(&fx), sizeof(float));
            file.write(reinterpret_cast<char*>(&fy), sizeof(float));
            file.write(reinterpret_cast<char*>(&fz), sizeof(float));
        }

        // Attribute byte count
        uint16_t attributeCount = 0;
        file.write(reinterpret_cast<char*>(&attributeCount), sizeof(attributeCount));
    }

    file.close();
    std::cout << "Binary STL file " << fileName << " has been created.\n";
}


double computeMaxRadius(const std::vector<Vec3>& points) {
    if (points.size() < 2) {
        std::cerr << "Error: At least two points are required to compute the radius.\n";
        return -1.0;
    }

    double minDist = std::numeric_limits<double>::max();
    size_t idx1 = 0, idx2 = 1;

    for (size_t i = 0; i < points.size(); i += 2) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            if (i == j) continue;
            Vec3 diff = points[i] - points[j];
            double dist = diff.length();
            if (dist >= minDist) continue;
            minDist = dist;
            idx1 = i;
            idx2 = j;
        }
    }

    Vec3 P1 = points[idx1];
    Vec3 P2 = points[idx2];

    double d1 = P1.length();
    double d2 = P2.length();

    if (d1 == 0 || d2 == 0) {
        std::cerr << "Error: One of the points is at the origin, undefined plane.\n";
        return -1.0;
    }

    Vec3 n1 = P1.normalize();
    Vec3 n2 = P2.normalize();

    Vec3 numeratorVec = (n1 * d2) - (n2 * d1);
    double numerator = numeratorVec.length();

    Vec3 crossProd = n1.cross(n2);
    double denominator = crossProd.length();

    if (denominator < 1e-6) {
        std::cerr << "Error: The two closest points define parallel or coincident planes.\n";
        return -1.0;
    }

    double r = numerator / denominator;

    return r;
}
