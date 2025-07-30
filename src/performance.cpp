#include "performance.h"
#include <iostream>

// Static member to store the start time of the timer
std::chrono::high_resolution_clock::time_point Performance::startTime;

// Starts the timer by recording the current high-resolution time
void Performance::startTimer() {
    startTime = std::chrono::high_resolution_clock::now();
}

// Stops the timer, calculates the elapsed time in milliseconds, and returns it
double Performance::stopTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    return duration.count();
}

// Logs a message and the measured time (in milliseconds) to the console
void Performance::log(const std::string& message, double time) {
    std::cout << message << ": " << time << " ms\n";
}
