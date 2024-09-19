#include <iostream>
#include <ctime>
#include <cmath>
#include <thread>
#include "Die.h"
#include "OptimizationThread.h"
#include "qt/MainWindow.h"
#include "qt/DieVisualization.h"
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QPainter>
#include <QString>
#include <QMutexLocker>
#include <QImage>
#include "STL.h"


using namespace std;


int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    // Wait for the input window to close
    app.exec();

    unsigned int sides = mainWindow.getSideCount();
    if (sides == 0) return 0;

    std::srand(std::time(0));  // Seed the random number generator with the current time

    // Create array of 5 Die pointers
    std::array<Die*, THREAD_COUNT> dieArray{nullptr, nullptr, nullptr, nullptr, nullptr};

    // Initialize the "best" Die at index 4
    dieArray[THREAD_COUNT - 1] = new Die(sides, true);

    // Create the optimization threads
    std::vector<OptimizationThread*> optimizationThreads;
    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        this_thread::sleep_for(chrono::milliseconds(
                200));  //give slight head start to each thread to make less likely to jump around which thread is best
        OptimizationThread* thread = new OptimizationThread(i, dieArray, sides);
        optimizationThreads.push_back(thread);
        thread->start();
    }

    // Create a thread to keep optimizing the best Die (index 4)
    std::thread bestThread([&dieArray]() {
        while (true) {
            dieArray[THREAD_COUNT - 1]->optimize();
        }
    });

    // Create a thread to save best results
    std::thread saveThread([&dieArray]() {
        while (true) {
            this_thread::sleep_for(chrono::seconds(10));
            size_t bestIndex = 0;
            double bestStress = std::numeric_limits<double>::max();
            for (int i = 0; i < THREAD_COUNT; ++i) {
                if (dieArray[i] != nullptr) {
                    double stress = dieArray[i]->getBest().getTotalStress();
                    if (stress < bestStress) {
                        bestStress = stress;
                        bestIndex = i;
                    }
                }
            }
            dieArray[bestIndex]->save();
        }
    });

    // Create the visualization window
    DieVisualization visualization(dieArray);
    visualization.show();

    return app.exec();
}
