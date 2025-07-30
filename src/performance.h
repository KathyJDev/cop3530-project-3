#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <chrono>
#include <string>

// The Performance class provides static methods for timing operations
// and logging performance results.
class Performance {
    // Stores the start time for the timer (shared by all methods)
    static std::chrono::high_resolution_clock::time_point startTime;
public:
    // Starts the timer by recording the current time
    static void startTimer();

    // Stops the timer and returns the elapsed time in milliseconds
    static double stopTimer();

    // Logs a message and the measured time (in milliseconds) to the console
    static void log(const std::string& message, double time);
};

#endif // PERFORMANCE_H
