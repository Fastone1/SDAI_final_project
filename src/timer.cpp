#include "timer.hpp"

Timer::Timer() {
    start();
}

void Timer::reset() {
    start_time = std::chrono::steady_clock::time_point();
}

void Timer::set_start_time(int start_time) {
    this->start_time = std::chrono::steady_clock::time_point(std::chrono::seconds(start_time));
}

void Timer::set_end_time(int end_time) {
    this->end_time = std::chrono::steady_clock::time_point(std::chrono::seconds(end_time));
}

void Timer::start() {
    start_time = std::chrono::steady_clock::now();
}

void Timer::stop() {
    end_time = std::chrono::steady_clock::now();
}

double Timer::elapsed() const {
    return std::chrono::duration<double>(end_time - start_time).count();
}

double Timer::elapsed_ms() const {
    return std::chrono::duration<double, std::milli>(end_time - start_time).count();
}

double Timer::time() {
    stop();
    return elapsed();
}

double Timer::time_ms() {
    stop();
    return elapsed_ms();
}