#ifndef PT_H
#define PT_H

#include <iostream>
#include <chrono>
#include <string>
#include <map>

using namespace std;
using namespace std::chrono;

class PT {
public:
    PT(const std::string& label);
    ~PT();
    void stop();
    static void print();

private:
    struct TimingData {
        int calls = 0;
        long long totalTime = 0; // microseconds
    };

    static map<string, TimingData> _timingData;

    string _label;
    time_point<high_resolution_clock> _start;
    bool _stopped;
};

#endif // PT_H
