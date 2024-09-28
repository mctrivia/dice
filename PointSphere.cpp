//
// Created by mctrivia on 07/09/24.
//

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "PointSphere.h"
#include <limits>

/**
 * Generates a random point sphere of a specific number of sides
 * @param sideCount
 */
PointSphere::PointSphere(size_t sideCount) : _sideCount(sideCount) {
    //check even number of sides
    if (sideCount % 2 == 1) throw out_of_range("must be even number");

    //generate random start position
    int numPoints = _sideCount / 2;
    for (int i = 0; i < numPoints; ++i) {
        Vec3 point(static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0,
                   static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0,
                   static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0);
        point.normalize();
        _points.push_back(point);
    }
}

/**
 * Allow constructing of a PointSphere from another
 * @param other
 */
PointSphere::PointSphere(const PointSphere& other) {
    std::lock_guard<std::mutex> lock(other._mtx); // Lock the source mutex

    _sideCount = other._sideCount;
    _points = other._points;
    _lowestStressIndex = other._lowestStressIndex;
    _highestStressIndex = other._highestStressIndex;
    _totalStress = other._totalStress;
}

/**
 * Allow using the equal operator
 * @param other
 * @return
 */
PointSphere& PointSphere::operator=(const PointSphere& other) {
    if (this != &other) {
        const std::lock_guard<std::mutex> lockThis(_mtx);
        const std::lock_guard<std::mutex> lockOther(other._mtx);
        _sideCount = other._sideCount;
        _points = other._points;
        _lowestStressIndex = other._lowestStressIndex;
        _highestStressIndex = other._highestStressIndex;
        _totalStress = other._totalStress;
    }
    return *this;
}

/**
 * Load best known result
 */
double PointSphere::load() {
    double rate;

    //compute file name
    const string filename = "best/" + to_string(_sideCount) + ".csv";

    //get side count from name
    std::string baseName = filename.substr(5, filename.length() - 9);
    _sideCount = std::stoi(baseName);

    //make sure read and writes not at the same time
    const std::lock_guard<std::mutex> lock(_mtx);

    //check file exists
    ifstream inFile(filename);
    if (!inFile.is_open()) throw exception();

    //skip over stress value
    string line;
    getline(inFile, line);

    //load rate
    {
        getline(inFile, line);
        stringstream ss(line);
        string stressLabel;

        // Extract the stress value from the line, assuming it's after the "Stress: " label
        ss >> stressLabel >> rate;
    }

    //skip blank line
    getline(inFile, line);

    //get points
    _points.clear();  // Clear any existing data in _points
    while (getline(inFile, line)) {
        if (_points.size() == _sideCount / 2) break;
        stringstream ss(line);
        string token;
        double x, y, z;

        // Read X value
        if (getline(ss, token, ',')) {
            x = stod(token);
        }

        // Read Y value
        if (getline(ss, token, ',')) {
            y = stod(token);
        }

        // Read Z value
        if (getline(ss, token, ',')) {
            z = stod(token);
        }

        // Unscale the coordinates
        Vec3 point(x, y, z);

        _points.push_back(point);
    }

    inFile.close();
    return rate;
}

/**
 * Save the best result
 */
void PointSphere::save(double rate) {
    //compute file name
    const string filename = "best/" + to_string(_sideCount) + ".csv";

    //make sure read and writes not at the same time
    const std::lock_guard<std::mutex> lock(_mtx);

    //check better than saved vale
    double bestStress = getTotalStress(false);
    ifstream inFile(filename);
    if (inFile.is_open()) {
        string line;
        // Read the first line which contains the saved stress value
        if (getline(inFile, line)) {
            stringstream ss(line);
            string stressLabel;
            double savedStress;

            // Extract the stress value from the line, assuming it's after the "Stress: " label
            if (ss >> stressLabel >> savedStress) {
                // If the current best stress is greater than or equal to the saved value, return
                if (bestStress >= savedStress) {
                    inFile.close();
                    return;
                }
            }
        }
        inFile.close();
    }

    //set output type
    ofstream outFile(filename);
    outFile << fixed << setprecision(15);

    //write stress value
    outFile << "Stress: " << bestStress << endl;
    outFile << "Rate: " << rate << endl << endl;

    //write points
    if (outFile.is_open()) {
        for (const auto& point: _points) {
            outFile << point.x << "," << point.y << "," << point.z << endl;
        }
        outFile.close();
    } else {
        cerr << "Unable to open file for writing: " << filename << endl;
    }
}

/**
 * Gets a points location
 * @param sideIndex
 * @return
 */
Vec3 PointSphere::getPoint(size_t sideIndex) const {
    size_t index = sideIndex / 2;
    int multiplier = (sideIndex % 2 == 0) ? 1 : -1;
    return _points[index] * multiplier;
}

/**
 * Gets the stress on a point
 * @param sideIndex
 * @return
 */
Vec3 PointSphere::getStress(size_t sideIndex, bool lockWhileExecuting) const {
    // Use a unique_ptr to conditionally hold the lock
    std::unique_ptr<std::lock_guard<std::mutex>> lockGuard;
    if (lockWhileExecuting) {
        lockGuard = std::make_unique<std::lock_guard<std::mutex>>(_mtx);
    }

    //calculate the stress on a point
    Vec3 totalStress(0.0, 0.0, 0.0);
    Vec3 referencePoint = getPoint(sideIndex);

    // Iterate over all points to compute the stress due to each one
    for (size_t j = 0; j < _sideCount; ++j) {
        Vec3 point = getPoint(j);
        if (point == referencePoint) continue;

        Vec3 direction = referencePoint - point;
        double distSquared = direction.lengthSquared();                 // Compute the squared distance
        Vec3 directionNormalized = direction / sqrt(distSquared);    // Normalize the direction vector
        totalStress += directionNormalized * (1.0 /
                                              distSquared);       // Add the stress (force) vectorially: magnitude is 1 / distSquared, in the direction of directionNormalized
    }

    return totalStress;
}

/**
 * Gets the total stress in the system
 * @return
 */
double PointSphere::getTotalStress(bool lockWhileExecuting) {
    // Use a unique_ptr to conditionally hold the lock
    std::unique_ptr<std::lock_guard<std::mutex>> lockGuard;
    if (lockWhileExecuting) {
        lockGuard = std::make_unique<std::lock_guard<std::mutex>>(_mtx);
    }

    // If the total stress has already been calculated, return it
    if (_totalStress != numeric_limits<double>::infinity()) return _totalStress;

    //calculate total stress
    _totalStress = 0.0;
    for (size_t i = 0; i < _sideCount; ++i) {
        Vec3 sideI = getPoint(i);

        // Consider the repulsion between point i and all other points
        for (size_t j = i + 1; j < _sideCount; ++j) {
            if (i == j) continue;
            Vec3 sideJ = getPoint(j);
            double distSquared = sideI.distanceSquared(sideJ);
            if (distSquared == 0) return std::numeric_limits<double>::infinity();
            _totalStress += 1.0 / distSquared;
        }
    }
    return _totalStress;
}

/**
 * Returns the number of sides
 * @return
 */
size_t PointSphere::sideCount() const {
    return _sideCount;
}

/**
 * Move a point to a specific location
 * @param sideIndex
 * @param value
 */
void PointSphere::movePoint(size_t sideIndex, const Vec3& value) {
    const std::lock_guard<std::mutex> lock(_mtx);
    size_t index = sideIndex / 2;
    int mult = (sideIndex % 2 == 0) ? 1 : -1;  //handle if mirrored point was moved
    Vec3 newValue = value * mult;
    newValue.normalize();               //make sure its on sphere

    _points[index] = newValue;

    //clear caches
    _lowestStressIndex = numeric_limits<size_t>::max();
    _highestStressIndex = numeric_limits<size_t>::max();
    _totalStress = numeric_limits<double>::infinity();
}

size_t PointSphere::getHighestStressIndex() {
    const std::lock_guard<std::mutex> lock(_mtx);
    if (_highestStressIndex != numeric_limits<size_t>::max()) return _highestStressIndex;

    double stress = 0;
    for (size_t i = 0; i < _sideCount; i += 2) {    //skip every other since values will be identical
        double currentStress = getStress(i,
                                         false).lengthSquared();  //don't care about actual value so use faster squared value
        if (currentStress <= stress) continue;
        stress = currentStress;
        _highestStressIndex = i;
    }
    return _highestStressIndex;
}

size_t PointSphere::getLowestStressIndex() {
    const std::lock_guard<std::mutex> lock(_mtx);
    if (_lowestStressIndex != numeric_limits<size_t>::max()) return _lowestStressIndex;

    double stress = numeric_limits<double>::max();
    for (size_t i = 0; i < _sideCount; i += 2) {    //skip every other since values will be identical
        double currentStress = getStress(i,
                                         false).lengthSquared();  //don't care about actual value so use faster squared value
        if (currentStress >= stress) continue;
        stress = currentStress;
        _lowestStressIndex = i;
    }
    return _lowestStressIndex;
}

