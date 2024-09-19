//
// Created by mctrivia on 07/09/24.
//

#ifndef DICE_DIE_H
#define DICE_DIE_H


#include <vector>
#include <mutex>
#include <condition_variable>
#include "Vec3.h"
#include <chrono>
#include <QPainter>
#include "PointSphere.h"

//1 in RANDOM_RATE optimizations will be of random point rest will be on max stress
#define RANDOM_RATE 2

#define REDUCE_RATE 30

using namespace std;

class Die {
    double _moveRate;
    double _moveRateMin;
    PointSphere _best;
    PointSphere _current;
    std::chrono::steady_clock::time_point _lastBestTime;
    long _nextReduceTime = REDUCE_RATE;
    static bool _optimizationPaused;
    vector<size_t> _labels;
    size_t _lastOptimizedIndex=0;

public:
    Die(size_t sides, bool loadBest = false);
    void optimize();
    PointSphere getBest() const;
    void reduceRate();
    long getSecondsSinceLastBest() const;

    static void pauseOptimization();
    static void resumeOptimization();
    static bool isOptimizationPaused();

    void save();


    void draw(QPainter& img, bool highlightExtremes=true);
    vector<size_t> getLabels();
};



#endif //DICE_DIE_H
