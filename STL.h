// STL.h
#ifndef STL_H
#define STL_H

#include <vector>
#include <string>
#include <fstream>
#include "Vec3.h"



void writeTriangle(std::ofstream& file, const Vec3& normal, const Vec3& v1, const Vec3& v2, const Vec3& v3);
void createSTL(double r, const std::vector<Vec3>& points, const std::string& fileName);
double computeMaxRadius(const std::vector<Vec3>& points);

#endif // STL_H