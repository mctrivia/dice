#include "Vec3.h"
#include <cmath>
#include <iomanip>
#include <iostream>

// Constructor
Vec3::Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

// Normalize the vector to unit length
Vec3& Vec3::normalize() {
    double len = length();
    if (len > 0) {
        x /= len;
        y /= len;
        z /= len;
    }
    return *this;
}

// Return the length of the vector
double Vec3::length() const {
    return sqrt(x * x + y * y + z * z);
}

// Calculate the squared length of the vector
double Vec3::lengthSquared() const {
    return x * x + y * y + z * z;
}

// Calculate the squared distance between two points
double Vec3::distanceSquared(const Vec3& other) const {
    return (x - other.x) * (x - other.x) +
           (y - other.y) * (y - other.y) +
           (z - other.z) * (z - other.z);
}

// Return the distance between two points
double Vec3::distance(const Vec3& other) const {
    return sqrt(distanceSquared(other));
}

// Add two vectors
Vec3 Vec3::operator+(const Vec3& other) const {
    return Vec3(x + other.x, y + other.y, z + other.z);
}

// Add a vector to this vector (in place)
Vec3& Vec3::operator+=(const Vec3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

// Subtract two vectors
Vec3 Vec3::operator-(const Vec3& other) const {
    return Vec3(x - other.x, y - other.y, z - other.z);
}

// Multiply by a scalar
Vec3 Vec3::operator*(double scalar) const {
    return Vec3(x * scalar, y * scalar, z * scalar);
}

// Divide by a scalar
Vec3 Vec3::operator/(double scalar) const {
    return Vec3(x / scalar, y / scalar, z / scalar);
}

// Check equality of two vectors
bool Vec3::operator==(const Vec3& other) const {
    return (other.x == x && other.y == y && other.z == z);
}

bool Vec3::operator<(const Vec3& other) const {
    if (x != other.x) return x < other.x;
    if (y != other.y) return y < other.y;
    return z < other.z;
}

// Define the operator<< for Vec3
std::ostream& operator<<(std::ostream& os, const Vec3& vec) {
    os << std::fixed << std::setprecision(3) << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return os;
}

//todo generalize
double Vec3::angle(const Vec3& other) const {
    double dotProduct = x * other.x + y * other.y + z * other.z;
    // Since vectors are on a unit sphere, their lengths are 1
    // Clamp the dot product to the valid range [-1, 1]
    dotProduct = std::max(-1.0, std::min(1.0, dotProduct));
    return acos(dotProduct);
}

double Vec3::dot(const Vec3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

// Compute the cross product with another vector
Vec3 Vec3::cross(const Vec3& other) const {
    return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
    );
}