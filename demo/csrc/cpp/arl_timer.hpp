/*
 * @file: arl_timer.hpp
 * @date: 2022/05/31 21:00:46
 * @author: fuyi@sensetime.com
 * @brief:
 */

#ifndef ARL_TIMER_H
#define ARL_TIMER_H

#include <chrono>
#include <ratio>

class Timer {
    using clock = std::chrono::high_resolution_clock;

public:
    using h = std::ratio<3600>;
    using m = std::ratio<60>;
    using s = std::ratio<1, 1>;
    using ms = std::ratio<1, 1000>;
    using us = std::ratio<1, 1000000>;
    using ns = std::ratio<1, 1000000000>;

public:
    template <typename span, typename int_type = int64_t>
    static int_type get_timestamp() {
        int_type ts = std::chrono::duration_cast<std::chrono::duration<int_type, span>>(
                              clock::now().time_since_epoch())
                              .count();
        return ts;
    }

public:
    Timer() : _tp_start(clock::now()), _tp_stop(_tp_start) {}

public:
    void start() {
        _tp_start = clock::now();
    }
    void stop() {
        _tp_stop = clock::now();
    }
    template <typename span>
    auto delta() const {
        return std::chrono::duration<double, span>(clock::now() - _tp_start).count();
    }
    template <typename span>
    auto stop_delta() {
        stop();
        return std::chrono::duration<double, span>(_tp_stop - _tp_start).count();
    }
    template <typename span>
    auto stop_delta_start() {
        stop();
        auto ts = std::chrono::duration<double, span>(_tp_stop - _tp_start).count();
        start();
        return ts;
    }

private:
    std::chrono::time_point<clock> _tp_start;
    std::chrono::time_point<clock> _tp_stop;
};

#endif  // ARL_TIMER_H
