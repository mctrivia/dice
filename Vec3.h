#ifndef DICE_VEC3_H
#define DICE_VEC3_H

#include <cmath>
#include <iostream>

struct Vec3 {
    double x, y, z;

    // Constructor
    Vec3(double x_ = 0.0, double y_ = 0.0, double z_ = 0.0);

    // Normalize the vector to unit length
    Vec3& normalize();

    // Return the length of the vector
    double length() const;

    // Calculate the squared length of the vector (for performance when comparing distances)
    double lengthSquared() const;

    // Calculate the squared distance between two points
    double distanceSquared(const Vec3& other) const;

    // Return the distance between two points
    double distance(const Vec3& other) const;

    // Add two vectors
    Vec3 operator+(const Vec3& other) const;

    // Add a vector to this vector (in place)
    Vec3& operator+=(const Vec3& other);

    // Subtract two vectors
    Vec3 operator-(const Vec3& other) const;

    // Multiply by a scalar
    Vec3 operator*(double scalar) const;

    // Divide by a scalar
    Vec3 operator/(double scalar) const;

    bool operator==(const Vec3& other) const;

    bool operator<(const Vec3& other) const;

    // Compute the angle between this vector and another vector
    double angle(const Vec3& other) const;

    // Compute the dot product with another vector
    double dot(const Vec3& other) const;

    // Compute the cross product with another vector
    Vec3 cross(const Vec3& other) const;
};

// Forward declare the operator<<
std::ostream& operator<<(std::ostream& os, const Vec3& vec);

#endif // DICE_VEC3_H
