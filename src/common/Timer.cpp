
#include <paradem/timer.h>

#include <chrono>

void Timer::start() {
    start_time = clock::now();
}

double Timer::stop() {
    const auto end_time = clock::now();
    elapsed_time += difference( start_time, end_time );
    return elapsed_time;
}

double Timer::difference( const std::chrono::time_point< clock >& start, const std::chrono::time_point< clock >& end ) {
    return std::chrono::duration_cast< std::chrono::duration< double > >( end - start ).count();
}

double Timer::elapsed() {
    return elapsed_time;
}