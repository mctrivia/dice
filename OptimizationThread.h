// OptimizationThread.h
#ifndef OPTIMIZATIONTHREAD_H
#define OPTIMIZATIONTHREAD_H

#include <QThread>
#include <array>
#include <QMutex>
#include "Die.h"


//thread count must be at least 2
#define THREAD_COUNT 5

class OptimizationThread : public QThread {
Q_OBJECT
public:
    OptimizationThread(size_t index, std::array<Die*, THREAD_COUNT>& dieArray, unsigned int sides,
                       QObject* parent = nullptr);

protected:
    void run() override;

private:
    size_t index;
    std::array<Die*, THREAD_COUNT>& dieArray;
    unsigned int sides;
    QMutex* bestMutex;  //todo should they not be static so all objects use same lock
};

#endif // OPTIMIZATIONTHREAD_H
