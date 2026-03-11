#include <iostream>
#include <ctime>
#include <cmath>
#include <thread>
#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <limits>
#include "Die.h"
#include "OptimizationThread.h"
#include "qt/MainWindow.h"
#include "qt/DieVisualization.h"
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

using namespace std;

static QIcon createDiceIcon() {
    QPixmap pm(256, 256);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(30, 30, 60));
    p.setPen(QPen(QColor(100, 120, 200), 10));
    p.drawRoundedRect(12, 12, 232, 232, 40, 40);
    p.setBrush(QColor(220, 230, 255));
    p.setPen(Qt::NoPen);
    const int cx[2] = { 82, 174 };
    const int cy[3] = { 75, 128, 181 };
    for (int col = 0; col < 2; ++col)
        for (int row = 0; row < 3; ++row)
            p.drawEllipse(QPoint(cx[col], cy[row]), 24, 24);
    return QIcon(pm);
}

// ── Headless runner ───────────────────────────────────────────────────────────

static int runHeadless(unsigned int sides, int timeLimit) {
    std::srand(std::time(0));
    std::array<Die*, THREAD_COUNT> dieArray{nullptr,nullptr,nullptr,nullptr,nullptr};
    dieArray[THREAD_COUNT-1] = new Die(sides, true);

    std::atomic<bool> running(true);
    std::vector<OptimizationThread*> optThreads;
    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        this_thread::sleep_for(chrono::milliseconds(200));
        auto* t = new OptimizationThread(i, dieArray, sides, running);
        optThreads.push_back(t);
        t->start();
    }
    std::thread bestThread([&]() {
        while (running.load()) dieArray[THREAD_COUNT-1]->optimize();
    });
    std::thread saveThread([&]() {
        const int TICKS = 100; int tick = TICKS;
        while (running.load()) {
            this_thread::sleep_for(chrono::milliseconds(100));
            if (--tick > 0) continue;
            tick = TICKS;
            size_t best = 0; double bestStress = numeric_limits<double>::max();
            for (int i = 0; i < THREAD_COUNT; ++i)
                if (dieArray[i] && dieArray[i]->getBest().getTotalStress() < bestStress) {
                    bestStress = dieArray[i]->getBest().getTotalStress(); best = i; }
            dieArray[best]->save();
            double sec = dieArray[best]->getSecondsSinceLastBest();
            cout << "D" << dieArray[best]->getBest().sideCount()
                 << " " << sec << "s since best  stress="
                 << setprecision(15) << bestStress << "\n";
            if (timeLimit > 0 && sec >= timeLimit) {
                cout << "Time limit reached.\n";
                running.store(false); exit(0);
            }
        }
    });
    bestThread.join(); saveThread.join();
    for (auto* t : optThreads) { t->wait(); delete t; }
    for (auto* d : dieArray)   delete d;
    return 0;
}

// ── GUI runner ────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    unsigned int sides     = 0;
    int          timeLimit = -1;
    bool         headless  = false;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg.find("-s=") == 0) { sides     = stoi(arg.substr(3)); headless = true; }
        else if (arg.find("-t=") == 0) { timeLimit = stoi(arg.substr(3)); headless = true; }
    }
    if (headless) {
        if (sides == 0) { cerr << "Headless mode requires -s=<sides>\n"; return 1; }
        return runHeadless(sides, timeLimit);
    }

    std::srand(std::time(0));
    QApplication app(argc, argv);
    app.setWindowIcon(createDiceIcon());
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "/..");

    // dieArray lives here for the whole session.
    // DieVisualization holds a reference to it and polls every 50 ms.
    // Optimization threads write into it after Start is clicked.
    std::array<Die*, THREAD_COUNT> dieArray{nullptr,nullptr,nullptr,nullptr,nullptr};

    MainWindow window(dieArray);
    window.resize(800, 600);
    window.show();

    // Optimization state — created on Start, cleaned up on quit.
    std::atomic<bool>              running{false};
    std::vector<OptimizationThread*> optThreads;
    std::thread bestThread;
    std::thread saveThread;

    QObject::connect(&window, &MainWindow::startRequested,
                     [&](unsigned int sides) {
        dieArray[THREAD_COUNT-1] = new Die(sides, true);
        running.store(true);

        for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
            this_thread::sleep_for(chrono::milliseconds(100));
            auto* t = new OptimizationThread(i, dieArray, sides, running);
            optThreads.push_back(t);
            t->start();
        }
        bestThread = std::thread([&]() {
            while (running.load()) dieArray[THREAD_COUNT-1]->optimize();
        });
        saveThread = std::thread([&]() {
            const int TICKS = 100; int tick = TICKS;
            while (running.load()) {
                this_thread::sleep_for(chrono::milliseconds(100));
                if (--tick > 0) continue;
                tick = TICKS;
                size_t best = 0; double bestStress = numeric_limits<double>::max();
                for (int i = 0; i < THREAD_COUNT; ++i)
                    if (dieArray[i] &&
                        dieArray[i]->getBest().getTotalStress() < bestStress) {
                        bestStress = dieArray[i]->getBest().getTotalStress();
                        best = i;
                    }
                dieArray[best]->save();
            }
        });
    });

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        if (!running.load()) return;

        // Save best result
        size_t best = 0; double bestStress = numeric_limits<double>::max();
        for (int i = 0; i < THREAD_COUNT; ++i)
            if (dieArray[i] &&
                dieArray[i]->getBest().getTotalStress() < bestStress) {
                bestStress = dieArray[i]->getBest().getTotalStress(); best = i; }
        if (dieArray[best]) dieArray[best]->save();

        running.store(false);
        for (auto* t : optThreads) t->wait();
        if (bestThread.joinable()) bestThread.join();
        if (saveThread.joinable()) saveThread.join();

        for (auto* t : optThreads) delete t;
        for (auto* d : dieArray)   delete d;
    });

    return app.exec();
}
