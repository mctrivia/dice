//
// Created by mctrivia on 07/09/24.
//

#ifndef DICE2_POINTSPHERE_H
#define DICE2_POINTSPHERE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include "Vec3.h"

using namespace std;

class PointSphere {
    mutable mutex _mtx;
    size_t _sideCount;
    vector<Vec3> _points;
    size_t _lowestStressIndex = numeric_limits<size_t>::max();
    size_t _highestStressIndex = numeric_limits<size_t>::max();
    double _totalStress = numeric_limits<double>::infinity();

public:
    //constructor
    explicit PointSphere(size_t sideCount);
    PointSphere(const PointSphere& other);
    PointSphere& operator=(const PointSphere& other);

    //file handler
    double load();
    void save(double rate);

    //getter
    Vec3 getPoint(size_t sideIndex) const;
    Vec3 getStress(size_t sideIndex, bool lockWhileExecuting = true) const;
    double getTotalStress(bool lockWhileExecuting = true);
    size_t sideCount() const;
    size_t getHighestStressIndex();
    size_t getLowestStressIndex();

    //setter
    void movePoint(size_t sideIndex, const Vec3& value);
};



#endif //DICE2_POINTSPHERE_H
