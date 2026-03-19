#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

class Timer {
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> end_time;

public:
    Timer();

    void reset();

    void set_start_time(int start_time);
    void set_end_time(int end_time);

    void start();
    void stop();

    double elapsed() const;
    double elapsed_ms() const;

    double time();
    double time_ms();
};

#endif // TIMER_HPP