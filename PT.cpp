#include "PT.h"
#include <iomanip>

// Initialize the static member
map<string, PT::TimingData> PT::_timingData;

// Constructor
PT::PT(const std::string& label) : _label(label), _start(high_resolution_clock::now()), _stopped(false) {
    if (_timingData.find(_label) == _timingData.end()) {
        _timingData[_label] = TimingData();
    }
    _timingData[_label].calls++;
}

// Destructor
PT::~PT() {
    if (!_stopped) {
        stop();
    }
}

// Stop the timer
void PT::stop() {
    if (!_stopped) {
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - _start).count();
        _timingData[_label].totalTime += duration;
        _stopped = true;
    }
}

// Print the timing data
void PT::print() {
    const int labelWidth = 60;
    const int callsWidth = 10;
    const int avgTimeWidth = 15;
    const int totalTimeWidth = 15;

    cout << left << setw(labelWidth) << "Label"
         << setw(callsWidth) << "Calls"
         << setw(avgTimeWidth) << "Average Time (us)"
         << setw(totalTimeWidth) << "Total Time (us)" << endl;

    cout << string(labelWidth + callsWidth + avgTimeWidth + totalTimeWidth, '-') << endl;

    for (const auto& entry: _timingData) {
        const std::string& label = entry.first;
        const TimingData& data = entry.second;

        double averageTime = static_cast<double>(data.totalTime) / data.calls;

        cout << left << setw(labelWidth) << label
             << setw(callsWidth) << data.calls
             << setw(avgTimeWidth) << fixed << setprecision(2) << averageTime
             << setw(totalTimeWidth) << data.totalTime << endl;
    }
}
