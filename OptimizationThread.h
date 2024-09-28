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
    OptimizationThread(size_t index, std::array<Die*, THREAD_COUNT>& dieArray, unsigned int sides, std::atomic<bool>& running,
                       QObject* parent = nullptr);

protected:
    void run() override;

private:
    size_t _index;
    std::array<Die*, THREAD_COUNT>& _dieArray;
    unsigned int _sides;
    QMutex* _bestMutex;
    std::atomic<bool>& _running;
};

#endif // OPTIMIZATIONTHREAD_H
