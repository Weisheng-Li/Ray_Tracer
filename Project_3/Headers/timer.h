#pragma once

#include <chrono>
#include <iostream>

class Timer
{
public:
	static double duration_s;

	Timer()
	{
		startTimePoint = std::chrono::high_resolution_clock::now();
	}

	void Stop()
	{
		auto endTimePoint = std::chrono::high_resolution_clock::now();

		auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(startTimePoint).time_since_epoch().count();
		auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(endTimePoint).time_since_epoch().count();

		auto duration = end - start;
		duration_s = duration * 0.001; // Convert to second

		std::cout << "Elapsed time: " << duration_s << " seconds\n" << std::endl;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
};

// define static member
double Timer::duration_s = 0.0f;