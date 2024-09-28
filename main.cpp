#include <iostream>
#include <ctime>
#include <cmath>
#include <thread>
#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include "Die.h"
#include "OptimizationThread.h"
#include "qt/MainWindow.h"
#include "qt/DieVisualization.h"
#include <QApplication>
#include <QTimer>
#include <iomanip>

using namespace std;


int main(int argc, char* argv[]) {
    // Variables for headless mode
    unsigned int sides = 0;
    int timeLimit = -1; // -1 indicates no time limit (run indefinitely)
    bool headlessMode = false;

    // Parse command-line arguments for -s (sides) and -t (time limit)
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg.find("-s=") == 0) {
            sides = stoi(arg.substr(3));
            headlessMode = true;
        } else if (arg.find("-t=") == 0) {
            timeLimit = stoi(arg.substr(3));
            headlessMode = true;
        }
    }

    // Seed the random number generator
    std::srand(std::time(0));

    // Create array of Die pointers
    std::array<Die*, THREAD_COUNT> dieArray{nullptr, nullptr, nullptr, nullptr, nullptr};

    // If headless mode and sides not specified, default to some value or handle error
    if (headlessMode && sides == 0) {
        std::cerr << "Headless mode requires -s=<sides> argument." << std::endl;
        return 1;
    }

    // If not headless, initialize GUI components first
    QApplication* app = nullptr;
    MainWindow* mainWindow = nullptr;
    DieVisualization* visualization = nullptr;

    if (!headlessMode) {
        app = new QApplication(argc, argv);
        mainWindow = new MainWindow();
        mainWindow->show();

        // Wait for the input window to close
        app->exec();

        sides = mainWindow->getSideCount();
        if (sides == 0) return 0;
    }

    // Initialize the "best" Die at the last index
    dieArray[THREAD_COUNT - 1] = new Die(sides, true);

    // Create the optimization threads
    std::atomic<bool> running(true);
    std::vector<OptimizationThread*> optimizationThreads;
    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        this_thread::sleep_for(chrono::milliseconds(200));  // slight delay for staggered starts
        OptimizationThread* thread = new OptimizationThread(i, dieArray, sides, running);
        optimizationThreads.push_back(thread);
        thread->start();
    }

    // Create a thread to keep optimizing the best Die (index THREAD_COUNT - 1)
    std::thread bestThread([&dieArray, &running]() {
        while (running.load()) {
            dieArray[THREAD_COUNT - 1]->optimize();
        }
    });

    // Create a thread to save best results
    std::thread saveThread([&dieArray, &timeLimit, headlessMode, &running]() {
        const char DECASECONDS_BETWEEN_SAVE = 100;
        char delayTics = DECASECONDS_BETWEEN_SAVE;
        while (running.load()) {
            this_thread::sleep_for(chrono::milliseconds(100));
            delayTics--;
            if (delayTics > 0) continue;
            delayTics = DECASECONDS_BETWEEN_SAVE;

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

            if (headlessMode) {
                double secondsSinceBest = dieArray[bestIndex]->getSecondsSinceLastBest();
                std::cout << "calculating D" << dieArray[bestIndex]->getBest().sideCount() << " it has been "
                          << secondsSinceBest << "s since last best: stress=" << setprecision(15) << bestStress
                          << std::endl;

                if (timeLimit > 0 && secondsSinceBest >= timeLimit) {
                    std::cout << "Time limit reached, exiting." << std::endl;
                    running.store(false);
                    exit(0);
                }
            }
        }
    });

    // If headless, run without GUI
    if (headlessMode) {
        // Wait for the threads to finish (which will be indefinite or until time limit)
        bestThread.join();
        saveThread.join();
    } else {
        // Create the visualization window
        visualization = new DieVisualization(dieArray);
        visualization->show();

        // Run the GUI event loop
        app->exec();

        // Save the state after the window closes
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

        // Stop the threads after the window closes
        running.store(false);

        // Join all optimization threads
        for (auto* thread: optimizationThreads) {
            thread->wait();
        }

        // Join the best and save threads
        bestThread.join();
        saveThread.join();
    }

    // Cleanup dynamically allocated memory
    for (auto& die: dieArray) {
        delete die;
    }
    for (auto& thread: optimizationThreads) {
        delete thread;
    }
    if (app) delete app;
    if (mainWindow) delete mainWindow;
    if (visualization) delete visualization;

    return 0;
}
