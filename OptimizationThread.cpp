// OptimizationThread.cpp
#include "OptimizationThread.h"

OptimizationThread::OptimizationThread(size_t index, std::array<Die*, THREAD_COUNT>& dieArray, unsigned int sides,
                                       std::atomic<bool>& running,
                                       QObject* parent)
        : QThread(parent), _index(index), _dieArray(dieArray), _sides(sides), _running(running) {
    _bestMutex = new QMutex();
}

void OptimizationThread::run() {
    const size_t bestThreadIndex = THREAD_COUNT - 1;
    while (_running.load()) {
        Die* currentDie = new Die(_sides, false);

        {
            QMutexLocker locker(_bestMutex);
            if (_dieArray[_index] != nullptr) delete _dieArray[_index];
            _dieArray[_index] = currentDie;
        }

        while (_running.load() && (currentDie->getSecondsSinceLastBest() < 120)) {
            currentDie->optimize();
        }

        {
            QMutexLocker locker(_bestMutex);
            double currentStress = currentDie->getBest().getTotalStress();
            if (currentStress < _dieArray[bestThreadIndex]->getBest().getTotalStress()) {
                *(_dieArray[bestThreadIndex]) = *currentDie;
            }
        }
    }
}
