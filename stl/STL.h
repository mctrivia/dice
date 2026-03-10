#pragma once
#include <vector>
#include <string>
#include "../Vec3.h"
#include "Font.h"

void createSTL(double r, const std::vector<Vec3>& points, const std::string& fileName,
               const std::vector<size_t>& labels,
               FontStyle font       = FontStyle::Seg7,
               double engraveDepth  = 0.5,
               double draftAngleDeg = 1.0);

double computeMaxRadius(const std::vector<Vec3>& points);
