// OptimizationThread.cpp
#include "OptimizationThread.h"

OptimizationThread::OptimizationThread(size_t index, std::array<Die*, THREAD_COUNT>& dieArray, unsigned int sides,
                                       QObject* parent)
        : QThread(parent), index(index), dieArray(dieArray), sides(sides) {
    bestMutex = new QMutex();
}

void OptimizationThread::run() {
    const size_t bestThreadIndex = THREAD_COUNT - 1;
    while (true) {
        Die* currentDie = new Die(sides, false);

        {
            QMutexLocker locker(bestMutex);
            if (dieArray[index] != nullptr) delete dieArray[index];
            dieArray[index] = currentDie;
        }

        while (currentDie->getSecondsSinceLastBest() < 120) {
            currentDie->optimize();
        }

        {
            QMutexLocker locker(bestMutex);
            double currentStress = currentDie->getBest().getTotalStress();
            if (currentStress < dieArray[bestThreadIndex]->getBest().getTotalStress()) {
                *(dieArray[bestThreadIndex]) = *currentDie;
            }
        }
    }
}
