#ifndef PARADEM_TIMER_H
#define PARADEM_TIMER_H

#include <chrono>


class Timer
{
private:
	using clock = std::chrono::high_resolution_clock;
	std::chrono::time_point<clock> start_time;
	double elapsed_time = 0;
	double difference(const std::chrono::time_point<clock> &start, const std::chrono::time_point<clock> &end);
	int gridRow;
	int gridCol;
public:
	void start();
	double stop();
	double elapsed();
};

#endif

