//
// Created by mctrivia on 07/09/24.
//

#include "Die.h"
#include <algorithm>
#include <cmath>
#include <set>

bool Die::_optimizationPaused = false;

/**
 * Create die object
 * @param sides
 * @param loadBest
 */
Die::Die(size_t sides, bool loadBest) : _best(sides), _current(sides),
                                        _lastBestTime(std::chrono::steady_clock::now()) {
    //set default start rates
    _moveRate = 0.1 / sides;
    _moveRateMin = 1 / sides / sides;

    //try to load best if requested
    if (!loadBest) return;
    try {
        _moveRate = _best.load();
        _current = _best;
        _moveRateMin = 1 / sides / sides;
    } catch (...) {
    }
}

/**
 * Try to optimize a point
 */
void Die::optimize() {
    if (isOptimizationPaused()) return; //don't optimize if paused

    size_t optimizeIndex;
    if (rand()%16==0) {
        //occasionally just pick one at random
        optimizeIndex = rand()%_current.sideCount();
    } else {
        // Store distances and indices
        std::vector<std::pair<double, size_t>> distances;

        // Get the point at _lastOptimizedIndex
        Vec3 referencePoint = _current.getPoint(_lastOptimizedIndex);

        // Compute the distances from the lowest stress point to all other points
        for (size_t i = 0; i < _current.sideCount(); ++i) {
            if (i == _lastOptimizedIndex) continue;  // Skip the point itself

            Vec3 currentPoint = _current.getPoint(i);
            double distance = referencePoint.distanceSquared(currentPoint);
            distances.emplace_back(distance, i);
        }

        // Sort distances from closest to farthest
        std::sort(distances.begin(), distances.end(),
                  [](const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) {
                      return a.first < b.first;  // Compare based on distances
                  });

        // Randomly pick one of the three closest points
        size_t randomIndex = rand() % static_cast<size_t>(sqrt(_current.sideCount()));

        // Set _lastOptimizedIndex to the index of the randomly selected closest point
        _lastOptimizedIndex = distances[randomIndex].second;
        optimizeIndex=_lastOptimizedIndex;
    }

    //compute how much to move point
    Vec3 maxStressPoint = _current.getPoint(optimizeIndex);
    Vec3 moveAmount = _current.getStress(optimizeIndex) * _moveRate;
    Vec3 newPoint = maxStressPoint + moveAmount;
    newPoint.normalize();

    //move the point
    _current.movePoint(optimizeIndex, newPoint);

    //see if best
    if (_current.getTotalStress() < _best.getTotalStress()) {
        _nextReduceTime = REDUCE_RATE;
        _best = _current;
        _lastBestTime = std::chrono::steady_clock::now();
        //_best.save(_moveRate);
        _labels.clear();    //remove cached labels since things have moved
        return;
    }

    //reduce rate if it has been a while
    if (getSecondsSinceLastBest() > _nextReduceTime) {
        _nextReduceTime += REDUCE_RATE;
        reduceRate();
    }
}

/**
 * Return best point sphere
 * @return
 */
PointSphere Die::getBest() const {
    return _best;
}

void Die::draw(QPainter& painter, bool highlightExtremes) {
    const int horizontalPadding = 15;

    QPaintDevice* device = painter.device();
    int imgWidth = device->width();
    int imgHeight = device->height();

    double height = imgHeight - 100;                    // Calculate the drawable height
    double width = imgWidth - 2 * horizontalPadding;    // Calculate the drawable width
    double spaceBetweenSpheres = (width - height * 3) / 2;

    // If not wide enough to paint full height, recalculate using width
    if (spaceBetweenSpheres < horizontalPadding) {
        spaceBetweenSpheres = horizontalPadding;
        height = (width - 2 * horizontalPadding) / 3;
    }

    // Draw each view
    for (int i = 0; i < 3; i++) {
        QPointF center(horizontalPadding + height / 2 + i * (height + spaceBetweenSpheres), imgHeight / 2);
        int radius = static_cast<int>(height / 2.0);

        // Draw the sphere outline
        painter.setPen(QPen(Qt::black, 2));
        painter.drawEllipse(center, radius, radius);

        // Draw curved latitude lines
        int numLatitudeLines = 20;
        for (int j = 1; j < numLatitudeLines; ++j) {
            double latAngle = M_PI * j / numLatitudeLines - M_PI / 2;  // Latitude angle (-90 to 90 degrees)
            int arcRadius = static_cast<int>(radius * cos(latAngle));  // Arc radius for the latitude line

            // Draw the latitude line as an ellipse (simulating a curved line)
            painter.setPen(QPen(QColor(200, 200, 200), 1));
            painter.drawEllipse(center, radius, arcRadius);
        }
        painter.drawLine(QPointF{center.x() - radius, center.y()}, QPointF{center.x() + radius, center.y()});

        // Draw curved longitude lines
        int numLongitudeLines = 20;
        for (int j = 1; j < numLongitudeLines; ++j) {
            double longAngle = M_PI * j / numLongitudeLines - M_PI / 2;  // Latitude angle (-90 to 90 degrees)
            int arcRadius = static_cast<int>(radius * cos(longAngle));  // Arc radius for the latitude line

            // Draw the latitude line as an ellipse (simulating a curved line)
            painter.setPen(QPen(QColor(200, 200, 200), 1));
            painter.drawEllipse(center, arcRadius, radius);
        }
        painter.drawLine(QPointF{center.x(), center.y() - radius}, QPointF{center.x(), center.y() + radius});
    }

    // Find the point with the highest and lowest stress
    size_t maxStressIndex = _best.getHighestStressIndex();
    size_t minStressIndex = _best.getLowestStressIndex();
    double maxStress = _best.getStress(maxStressIndex).length();
    double minStress = _best.getStress(minStressIndex).length();

    // Draw points for each view (front, top, side)
    for (int i = 0; i < 3; i++) {
        QPointF center(horizontalPadding + height / 2 + i * (height + spaceBetweenSpheres), imgHeight / 2);
        int radius = static_cast<int>(height / 2.0);

        // Draw the points on the sphere
        for (size_t j = 0; j < _best.sideCount(); ++j) {
            Vec3 point = _best.getPoint(j);

            double x, y, z;
            switch (i) {
                case 0:  // X-axis up (view from YZ plane)
                    x = center.x() + radius * point.y;
                    y = center.y() + radius * point.z;
                    z = point.x;
                    break;
                case 1:  // Y-axis up (view from XZ plane)
                    x = center.x() + radius * point.x;
                    y = center.y() + radius * point.z;
                    z = point.y;
                    break;
                case 2:  // Z-axis up (view from XY plane)
                    x = center.x() + radius * point.x;
                    y = center.y() + radius * point.y;
                    z = point.z;
                    break;
            }

            // Normalize stress between 0 and 1
            double stress = _best.getStress(j).length();
            double normalizedStress = (stress - minStress) / (maxStress - minStress);  // Value between 0 (least stress) and 1 (most stress)

            // Interpolate color between green (least stress) and red (most stress)
            int red = static_cast<int>(255 * normalizedStress);
            int green = static_cast<int>(255 * (1 - normalizedStress));
            QColor pointColor = QColor(red, green, 0);

            // Draw the point if it's in front
            if (z >= 0) {
                bool extreme=highlightExtremes&&((j/2 == minStressIndex/2) || (j/2 == maxStressIndex/2));
                painter.setPen(Qt::NoPen);
                painter.setBrush(pointColor);
                painter.drawEllipse(QPointF(x, y), extreme?3:2, extreme?3:2);
            }
        }
    }
}


void Die::reduceRate() {
    _moveRate /= 2;
    if (_moveRate < _moveRateMin) _moveRate = _moveRateMin;
}

long Die::getSecondsSinceLastBest() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - _lastBestTime).count();
}

void Die::pauseOptimization() {
    _optimizationPaused = true;
}

void Die::resumeOptimization() {
    _optimizationPaused = false;
}

bool Die::isOptimizationPaused() {
    return _optimizationPaused;
}

std::vector<size_t> Die::getLabels() {
    if (!_labels.empty()) return _labels;

    //labels not assigned so compute
    int N = static_cast<int>(_best.sideCount());
    std::vector<size_t> bestAssignment(N, 0);
    double maxTotalDistance = -1.0;
    const int numTrials = 100;  // Number of random guesses

    // Seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    for (int trial = 0; trial < numTrials; ++trial) {
        std::vector<size_t> assignedLabels(N, 0);
        std::vector<size_t> labelsToPoints(N + 1, 0);
        std::vector<size_t> unassignedPoints(N);
        for (int i = 0; i < N; ++i) {
            unassignedPoints[i] = i;
        }

        //create a lambda to handle assigning points
        auto assignLabel = [&assignedLabels, &labelsToPoints, &unassignedPoints, N](size_t randomIndex, size_t label) {
            //get point index
            size_t pointIndex = unassignedPoints[randomIndex];

            //compute its opposite
            size_t oppositeIndex = (2 * floor(pointIndex / 2) + (1 - pointIndex % 2));
            size_t oppositeLabel = N + 1 - label;

            //assign labels
            assignedLabels[pointIndex] = label;
            assignedLabels[oppositeIndex] = oppositeLabel;

            //erase from unassigned list
            unassignedPoints.erase(unassignedPoints.begin() + randomIndex);
            if (randomIndex % 2 == 1) {
                randomIndex--;
            }    //if we deleted second index in set then set to delete first.  if we deleted first second will have moved to this index
            unassignedPoints.erase(unassignedPoints.begin() + randomIndex);

            //save label to points map
            labelsToPoints[label] = pointIndex;
            labelsToPoints[oppositeLabel] = oppositeIndex;

            //returns selected index
            return pointIndex;
        };

        // Randomly select starting point
        int lastPointIndex = assignLabel(rand() % N, 1);

        //assign labels
        for (size_t label = 2; label <= N / 2; ++label) { //only do half because we assign 2 at a time
            // Find all points between 90 and 180 degrees away
            Vec3 lastPoint = _best.getPoint(lastPointIndex);
            std::vector<int> candidatePoints;
            double maxAngle = 0.0;
            int furthestPointIndex = 0;
            for (size_t i = 0; i < unassignedPoints.size(); i++) {
                size_t idx = unassignedPoints[i];

                Vec3 point = _best.getPoint(idx);
                //compute angle and keep if in range
                double angle = lastPoint.angle(point);
                if (angle >= M_PI / 2 && angle < M_PI) {
                    candidatePoints.push_back(i);
                }
                // Keep track of furthest point in case no candidate points
                if (angle > maxAngle) {
                    maxAngle = angle;
                    furthestPointIndex = i;
                }
            }

            //pick a point
            int selectedPointIndex;
            if (!candidatePoints.empty()) {
                // Select random point from candidatePoints
                int randomIndex = rand() % candidatePoints.size();
                selectedPointIndex = candidatePoints[randomIndex];
            } else {
                // No points between 90 and 180 degrees, select furthest point
                selectedPointIndex = furthestPointIndex;
            }

            lastPointIndex = assignLabel(selectedPointIndex, label);
        }

        // Compute total distance between consecutively labeled points
        double totalDistance = 0.0;
        for (int l = 1; l < N; ++l) {
            int idx1 = labelsToPoints[l];
            int idx2 = labelsToPoints[l + 1];
            Vec3 point1 = _best.getPoint(idx1);
            Vec3 point2 = _best.getPoint(idx2);
            totalDistance += point1.distance(point2);
        }

        if (totalDistance > maxTotalDistance) {
            maxTotalDistance = totalDistance;
            bestAssignment = assignedLabels;
        }
    }
    _labels = bestAssignment;

    return _labels;
}

void Die::save() {
    _best.save(_moveRate);
}
