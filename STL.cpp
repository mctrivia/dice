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
#include "STL.h"

// Hash and Equality functions for Vec3
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
    Vec3 normal;
    double d; // plane equation: normal.dot(point) = d

    Plane(const Vec3& normal_, double d_) : normal(normal_), d(d_) {}

    // Compute signed distance from point to plane
    double distance(const Vec3& point) const {
        return normal.dot(point) - d;
    }

    // Compute intersection point between edge (v0, v1) and plane
    // Assumes that v0 and v1 are on opposite sides of the plane
    Vec3 intersect(const Vec3& v0, const Vec3& v1) const {
        double dist0 = distance(v0);
        double dist1 = distance(v1);
        double t = dist0 / (dist0 - dist1);
        return v0 + (v1 - v0) * t;
    }
};

// Helper function to write a single triangle to the STL file
void writeTriangle(std::ofstream& file, const Vec3& normal, const Vec3& v1, const Vec3& v2, const Vec3& v3) {
    file << "  facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
    file << "    outer loop\n";
    file << "      vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
    file << "      vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
    file << "      vertex " << v3.x << " " << v3.y << " " << v3.z << "\n";
    file << "    endloop\n";
    file << "  endfacet\n";
}

// Function to clip a polygon (triangle) against a plane and collect intersection points
void clipPolygonWithPlane(const std::vector<Vec3>& polygon, const Plane& plane, std::vector<Vec3>& outPolygon, std::vector<Vec3>& intersectionPoints) {
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
                // Edge from outside to inside, add intersection point
                Vec3 intersectPoint = plane.intersect(prevVertex, currVertex);
                outPolygon.push_back(intersectPoint);
                intersectionPoints.push_back(intersectPoint);
            }
            // Add current vertex
            outPolygon.push_back(currVertex);
        } else if (prevInside) {
            // Edge from inside to outside, add intersection point
            Vec3 intersectPoint = plane.intersect(prevVertex, currVertex);
            outPolygon.push_back(intersectPoint);
            intersectionPoints.push_back(intersectPoint);
        }
        // Else, both outside, do nothing
    }
}

void createSTL(double r, const std::vector<Vec3>& points, const std::string& fileName) {
    // Number of divisions for latitude and longitude
    const int latDiv = 100;  // Increase divisions for smoother sphere
    const int lonDiv = 100;

    // Generate sphere vertices
    std::vector<Vec3> vertices;
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
    for (int i = 0; i < latDiv; ++i) {
        for (int j = 0; j < lonDiv; ++j) {
            int first = i * (lonDiv + 1) + j;
            int second = first + lonDiv + 1;

            // First triangle
            faces.push_back({first, second, first + 1});

            // Second triangle
            faces.push_back({second, second + 1, first + 1});
        }
    }

    // Prepare the cutting planes
    struct CuttingPlane {
        Plane plane;
        Vec3 point; // point used to define the plane
    };
    std::vector<CuttingPlane> cuttingPlanes;

    for (const Vec3& point : points) {
        Vec3 normal = point;
        normal.normalize();
        double d = normal.dot(point);
        cuttingPlanes.push_back({Plane(normal, d), point});
    }

    // Open the STL file for writing
    std::ofstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Unable to open file " << fileName << " for writing.\n";
        return;
    }

    file << "solid sphere\n";

    // For each cutting plane, maintain a list of intersection points
    std::vector<std::vector<Vec3>> planeBoundaries(cuttingPlanes.size());

    // Process each face and apply the cuts
    for (const auto& faceIndices : faces) {
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
            clipPolygonWithPlane(clippedPolygon, cuttingPlanes[p].plane, tempPolygon, intersectionPoints);

            // Collect intersection points
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
        // For n-vertex polygon, create n-2 triangles
        for (size_t i = 1; i + 1 < clippedPolygon.size(); ++i) {
            Vec3 v0 = clippedPolygon[0];
            Vec3 v1 = clippedPolygon[i];
            Vec3 v2 = clippedPolygon[i+1];

            // Compute normal
            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            Vec3 faceNormal = edge1.cross(edge2);
            faceNormal.normalize();

            // Write triangle
            writeTriangle(file, faceNormal, v0, v1, v2);
        }
    }

    // For each cutting plane, create caps
    for (size_t p = 0; p < cuttingPlanes.size(); ++p) {
        auto& intersectionPoints = planeBoundaries[p];

        if (intersectionPoints.empty()) {
            continue;
        }

        // Remove duplicate points
        std::vector<Vec3> boundaryPoints;
        std::unordered_set<Vec3, Vec3Hash, Vec3Equal> pointSet;
        for (const auto& pt : intersectionPoints) {
            if (pointSet.find(pt) == pointSet.end()) {
                boundaryPoints.push_back(pt);
                pointSet.insert(pt);
            }
        }

        // Project boundary vertices onto 2D plane for sorting
        Vec3 normal = cuttingPlanes[p].plane.normal;
        Vec3 center = cuttingPlanes[p].point;

        Vec3 u;
        if (fabs(normal.x) > 1e-6 || fabs(normal.y) > 1e-6) {
            u = Vec3(-normal.y, normal.x, 0).normalize();
        } else {
            u = Vec3(1, 0, 0);
        }
        Vec3 v = normal.cross(u);

        // Compute angles of boundary points around center
        std::vector<std::pair<double, Vec3>> anglePointPairs;
        for (const auto& vertex : boundaryPoints) {
            Vec3 vec = vertex - center;
            double x = vec.dot(u);
            double y = vec.dot(v);
            double angle = atan2(y, x);
            anglePointPairs.emplace_back(angle, vertex);
        }

        // Sort the points by angle
        std::sort(anglePointPairs.begin(), anglePointPairs.end());

        // Reconstruct the ordered boundary loop
        std::vector<Vec3> boundaryLoop;
        for (const auto& ap : anglePointPairs) {
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

            // Write triangle
            writeTriangle(file, normal, v0, v1, v2);
        }
    }

    file << "endsolid sphere\n";
    file.close();
    std::cout << "STL file " << fileName << " has been created.\n";
}

double computeMaxRadius(const std::vector<Vec3>& points) {
    if (points.size() < 2) {
        std::cerr << "Error: At least two points are required to compute the radius.\n";
        return -1.0;
    }

    // Step 1: Find the two closest points
    double minDist = std::numeric_limits<double>::max();
    size_t idx1 = 0, idx2 = 1;

    for (size_t i = 0; i < points.size(); i+=2) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            if (i==j) continue;
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

    // Step 2: Define the normals and distances for the two planes
    double d1 = P1.length();
    double d2 = P2.length();

    if (d1 == 0 || d2 == 0) {
        std::cerr << "Error: One of the points is at the origin, undefined plane.\n";
        return -1.0;
    }

    Vec3 n1 = P1.normalize();
    Vec3 n2 = P2.normalize();

    // Step 3: Compute the numerator and denominator for the radius formula
    Vec3 numeratorVec = (n1 * d2) - (n2 * d1);
    double numerator = numeratorVec.length();

    Vec3 crossProd = n1.cross(n2);
    double denominator = crossProd.length();

    if (denominator < 1e-6) { // Check if planes are parallel or coincident
        std::cerr << "Error: The two closest points define parallel or coincident planes.\n";
        return -1.0;
    }

    // Step 4: Compute the radius
    double r = numerator / denominator;

    return r;
}